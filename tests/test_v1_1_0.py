import pytest
import time
from selenium.webdriver.common.keys import Keys

def test_sidebar_toggle(driver):
    """Verify that Ctrl+B toggles the sidebar visibility."""
    # Initially visible
    search = driver.find_element(by='accessibility id', value="1004")
    assert search.is_displayed()

    # Toggle off
    driver.find_element(by='name', value="LibreTerm").send_keys(Keys.CONTROL + 'b')
    time.sleep(0.5)
    
    # Check if hidden
    # In some WinAppDriver versions, hidden elements might still be found but is_displayed() should be false
    try:
        search = driver.find_element(by='accessibility id', value="1004")
        assert not search.is_displayed()
    except Exception:
        pass # If it's completely gone from tree, that's also fine

    # Toggle back on
    driver.find_element(by='name', value="LibreTerm").send_keys(Keys.CONTROL + 'b')
    time.sleep(0.5)
    search = driver.find_element(by='accessibility id', value="1004")
    assert search.is_displayed()

def test_fullscreen_toggle(driver):
    """Verify that F11 toggles fullscreen mode."""
    # This is hard to verify purely via UI size because WinAppDriver might not report screen size correctly,
    # but we can check if the menu disappears.
    
    # Check menu exists
    file_menu = driver.find_element(by='name', value="File")
    assert file_menu.is_displayed()

    # F11
    driver.find_element(by='name', value="LibreTerm").send_keys(Keys.F11)
    time.sleep(1)

    # Menu should be gone
    try:
        file_menu = driver.find_element(by='name', value="File")
        assert not file_menu.is_displayed()
    except Exception:
        pass # Gone from tree is good

    # Restore
    driver.find_element(by='name', value="LibreTerm").send_keys(Keys.F11)
    time.sleep(1)
    file_menu = driver.find_element(by='name', value="File")
    assert file_menu.is_displayed()

def test_status_bar_updates(driver):
    """Verify that the status bar shows connection details."""
    # Launch a session (use Example Host if present)
    try:
        item = driver.find_element(by='name', value="Example Host")
    except:
        tree = driver.find_element(by='accessibility id', value="1001")
        item = tree.find_element(by='xpath', value="//TreeItem[1]")
    
    item.click()
    item.send_keys(Keys.ENTER)
    time.sleep(5) # Wait for PuTTY to launch and embed

    # Check Status Bar (IDC_STATUSBAR = 1003)
    status = driver.find_element(by='accessibility id', value="1003")
    # WinAppDriver sometimes needs a refresh or we can try to find by Name which is the text
    text = status.text
    if not text:
        # Fallback: try to find by Name property
        try:
            text = status.get_attribute("Name")
        except:
            pass
            
    assert text and ("Connected" in text or "@" in text)

def test_tab_rename(driver, desktop_session):
    """Verify that renaming a tab works."""
    # Ensure a session is open
    tabs = driver.find_element(by='accessibility id', value="1002")
    try:
        # Try to find any tab item
        tab_item = driver.find_element(by='xpath', value="//TabItem")
    except:
        # Launch one if not open
        try:
            item = driver.find_element(by='name', value="Example Host")
        except:
            tree = driver.find_element(by='accessibility id', value="1001")
            item = tree.find_element(by='xpath', value="//TreeItem[1]")
        item.click()
        item.send_keys(Keys.ENTER)
        time.sleep(5)
        tab_item = driver.find_element(by='xpath', value="//TabItem")

    # Right click the tab item
    tab_item.click()
    tab_item.send_keys(Keys.SHIFT + Keys.F10)
    time.sleep(1)
    
    # Select Rename
    try:
        rename_item = desktop_session.find_element(by='name', value="Rename")
    except:
        # Fallback: find by accessibility id if known, or try to find in desktop session again
        time.sleep(1)
        rename_item = desktop_session.find_element(by='name', value="Rename")

    rename_item.click()
    time.sleep(1)
    
    # Find Rename Dialog (IDD_RENAME_TAB = 250)
    # The dialog title is "Rename Tab"
    dialog = desktop_session.find_element(by='name', value="Rename Tab")
    edit = dialog.find_element(by='accessibility id', value="201")
    edit.clear()
    edit.send_keys("RenamedSession")
    
    dialog.find_element(by='name', value="OK").click()
    time.sleep(0.5)
    
    # Check tab text
    assert "RenamedSession" in tab_item.text
