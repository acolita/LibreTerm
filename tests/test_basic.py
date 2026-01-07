import pytest
import os
import time
from appium import webdriver
from appium.options.common.base import AppiumOptions
from selenium.webdriver.remote.command import Command as RemoteCommand
from selenium.webdriver.remote.webelement import WebElement

class WinAppDriver(webdriver.Remote):
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

        payload = { "desiredCapabilities": clean_caps }
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

        # Normalize the internal dict structures
        if isinstance(response, dict) and 'value' in response:
            normalize(response['value'])
            
            # For FIND_ELEMENT commands, Selenium 4 Remote.execute returns the 'value' part directly
            # but we need to ensure the high-level find_element returns a WebElement.
            # If the response['value'] is a dict with the W3C key, we can wrap it.
            if driver_command in [RemoteCommand.FIND_ELEMENT, RemoteCommand.FIND_CHILD_ELEMENT]:
                if isinstance(response['value'], dict) and w3c_key in response['value']:
                    # We wrap it here so the return value of find_element is a WebElement
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

@pytest.fixture(scope='module')
def driver():
    app_path = os.path.abspath("build/LibreTerm.exe")
    options = AppiumOptions()
    options.set_capability("app", app_path)
    options.set_capability("platformName", "Windows")
    options.set_capability("deviceName", "WindowsPC")
    
    driver = WinAppDriver(command_executor='http://127.0.0.1:4723', options=options)
    yield driver
    driver.quit()

@pytest.fixture(scope='module')
def desktop_session():
    options = AppiumOptions()
    options.set_capability("app", "Root")
    options.set_capability("platformName", "Windows")
    options.set_capability("deviceName", "WindowsPC")
    
    driver = WinAppDriver(command_executor='http://127.0.0.1:4723', options=options)
    yield driver
    driver.quit()

def test_launch(driver):
    """Verify the application launches and shows the main window title."""
    assert "LibreTerm" in driver.title

def test_tree_exists(driver):
    """Verify the Connections tree view exists using its AutomationId."""
    # IDC_TREEVIEW = 1001
    tree = driver.find_element(by='accessibility id', value="1001")
    assert tree is not None
    assert isinstance(tree, WebElement)

def test_search_box_exists(driver):
    """Verify the Search box exists using its AutomationId."""
    # IDC_SEARCH_EDIT = 1004
    search = driver.find_element(by='accessibility id', value="1004")
    assert search is not None
    assert isinstance(search, WebElement)

def test_new_connection_dialog(driver, desktop_session):
    """Verify opening the New Connection dialog."""
    # Find "File" menu
    file_menu = driver.find_element(by='name', value="File")
    file_menu.click()
    
    time.sleep(0.5)
    
    # Find "New Connection" item from the Desktop session (as it's a popup)
    try:
        new_conn = desktop_session.find_element(by='name', value="New Connection")
        new_conn.click()
    except:
        # Fallback: send keys
        file_menu.send_keys('n')

    time.sleep(1)
    
    # Find Dialog
    try:
        dialog = driver.find_element(by='name', value="Connection Details")
    except:
        dialog = desktop_session.find_element(by='name', value="Connection Details")
        
    assert dialog is not None
    
    # Close it
    cancel = dialog.find_element(by='name', value="Cancel")
    cancel.click()