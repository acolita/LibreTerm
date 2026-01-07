import pytest
import os
from appium import webdriver
from appium.options.common.base import AppiumOptions
from selenium.webdriver.remote.command import Command as RemoteCommand


class WinAppDriver(webdriver.Remote):
    """Custom WinAppDriver class to handle legacy JSONWP format."""

    def start_session(self, capabilities, browser_profile=None):
        if hasattr(capabilities, 'to_capabilities'):
            caps = capabilities.to_capabilities()
        else:
            caps = capabilities

        clean_caps = {}
        for k, v in caps.items():
            if k.startswith("appium:"):
                clean_caps[k[7:]] = v
            else:
                clean_caps[k] = v

        payload = {"desiredCapabilities": clean_caps}
        response = self.execute(RemoteCommand.NEW_SESSION, payload)

        if isinstance(response, dict):
            val = response.get('value', response)
            self.session_id = response.get('sessionId') or val.get('sessionId')
            if hasattr(self, '_caps'):
                self._caps = val
            else:
                self.caps = val

    def execute(self, driver_command, params=None):
        response = super().execute(driver_command, params)

        # WinAppDriver returns elements in legacy JSONWP format {'ELEMENT': 'uuid'}
        # Modern Selenium expects W3C format {'element-6066-11e4-a52e-4f735466cecf': 'uuid'}
        w3c_key = 'element-6066-11e4-a52e-4f735466cecf'

        def normalize(obj):
            if isinstance(obj, dict):
                if 'ELEMENT' in obj:
                    obj[w3c_key] = obj.pop('ELEMENT')
                for k, v in obj.items():
                    normalize(v)
            elif isinstance(obj, list):
                for item in obj:
                    normalize(item)

        if isinstance(response, dict) and 'value' in response:
            normalize(response['value'])

            if driver_command in [RemoteCommand.FIND_ELEMENT, RemoteCommand.FIND_CHILD_ELEMENT]:
                if isinstance(response['value'], dict) and w3c_key in response['value']:
                    response['value'] = self.create_web_element(response['value'][w3c_key])
            elif driver_command in [RemoteCommand.FIND_ELEMENTS, RemoteCommand.FIND_CHILD_ELEMENTS]:
                if isinstance(response['value'], list):
                    wrapped_list = []
                    for item in response['value']:
                        if isinstance(item, dict) and w3c_key in item:
                            wrapped_list.append(self.create_web_element(item[w3c_key]))
                        else:
                            wrapped_list.append(item)
                    response['value'] = wrapped_list

        return response


@pytest.fixture(scope='session')
def driver(request):
    """Application driver fixture with cleanup on failure."""
    app_path = os.path.abspath("build/LibreTerm.exe")
    options = AppiumOptions()
    options.set_capability("app", app_path)
    options.set_capability("platformName", "Windows")
    options.set_capability("deviceName", "WindowsPC")

    drv = WinAppDriver(command_executor='http://127.0.0.1:4723', options=options)

    def cleanup():
        try:
            drv.quit()
        except Exception:
            pass

    request.addfinalizer(cleanup)
    return drv


@pytest.fixture(scope='session')
def desktop_session(request):
    """Desktop session fixture for accessing popups and menus."""
    options = AppiumOptions()
    options.set_capability("app", "Root")
    options.set_capability("platformName", "Windows")
    options.set_capability("deviceName", "WindowsPC")

    drv = WinAppDriver(command_executor='http://127.0.0.1:4723', options=options)

    def cleanup():
        try:
            drv.quit()
        except Exception:
            pass

    request.addfinalizer(cleanup)
    return drv


@pytest.fixture(autouse=True)
def close_dialogs_on_failure(request, driver, desktop_session):
    """Fixture to close any open dialogs on test failure and ensure clean state."""
    yield
    # After test runs, always try to close any open dialogs to ensure clean state
    # This runs regardless of test outcome to prevent dialogs from affecting next tests
    from selenium.webdriver.common.keys import Keys
    import time

    # Check if test failed
    test_failed = getattr(request.node, 'rep_call', None)
    test_failed = test_failed.failed if test_failed else False

    if test_failed:
        # On failure, be more aggressive about cleanup
        for _ in range(5):
            try:
                # Try to find and close any dialog by pressing Escape
                active = driver.find_element(by='xpath', value="//*")
                active.send_keys(Keys.ESCAPE)
                time.sleep(0.2)
            except Exception:
                pass

            # Also try through desktop session
            try:
                desktop_session.find_element(by='name', value="LibreTerm").send_keys(Keys.ESCAPE)
                time.sleep(0.2)
            except Exception:
                pass

        # Try to click away any menus by clicking on main window
        try:
            main_window = driver.find_element(by='name', value="LibreTerm")
            main_window.click()
        except Exception:
            pass


@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    """Hook to capture test result for use in fixtures."""
    outcome = yield
    rep = outcome.get_result()
    setattr(item, f"rep_{rep.when}", rep)


@pytest.fixture
def clean_connections():
    """Fixture to clean up connections.ini before/after test."""
    appdata = os.getenv('APPDATA')
    if appdata:
        conn_file = os.path.join(appdata, 'LibreTerm', 'connections.ini')
        if os.path.exists(conn_file):
            os.remove(conn_file)
    yield
    if appdata:
        conn_file = os.path.join(appdata, 'LibreTerm', 'connections.ini')
        if os.path.exists(conn_file):
            os.remove(conn_file)


@pytest.fixture
def clean_credentials():
    """Fixture to clean up credentials.ini before/after test."""
    appdata = os.getenv('APPDATA')
    if appdata:
        cred_file = os.path.join(appdata, 'LibreTerm', 'credentials.ini')
        if os.path.exists(cred_file):
            os.remove(cred_file)
    yield
    if appdata:
        cred_file = os.path.join(appdata, 'LibreTerm', 'credentials.ini')
        if os.path.exists(cred_file):
            os.remove(cred_file)
