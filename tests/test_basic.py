import pytest
import time
from selenium.webdriver.remote.webelement import WebElement


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
    except Exception:
        # Fallback: send keys
        file_menu.send_keys('n')

    time.sleep(1)

    # Find Dialog
    try:
        dialog = driver.find_element(by='name', value="Connection Details")
    except Exception:
        dialog = desktop_session.find_element(by='name', value="Connection Details")

    assert dialog is not None

    # Close it
    cancel = dialog.find_element(by='name', value="Cancel")
    cancel.click()
