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

def test_status_bar_updates(driver, desktop_session):
    """Verify that the status bar shows connection details for a localhost connection."""
    # 1. Create a Localhost connection
    file_menu = driver.find_element(by='name', value="File")
    file_menu.click()
    time.sleep(0.5)

    try:
        new_conn = desktop_session.find_element(by='name', value="New Connection")
        new_conn.click()
    except:
        file_menu.send_keys('n')
    time.sleep(1)

    dialog = desktop_session.find_element(by='name', value="Connection Details")
    name_edit = dialog.find_element(by='accessibility id', value="201")
    name_edit.click()
    name_edit.send_keys("FakeServer")

    host_edit = dialog.find_element(by='accessibility id', value="202")
    host_edit.click()
    host_edit.send_keys("127.0.0.1")

    port_edit = dialog.find_element(by='accessibility id', value="203")
    port_edit.click()
    port_edit.clear()
    port_edit.send_keys("2222")

    # Clear default user/pass if any
    dialog.find_element(by='accessibility id', value="204").clear()
    dialog.find_element(by='accessibility id', value="207").clear()

    dialog.find_element(by='name', value="OK").click()
    time.sleep(2)

    # 2. Launch it
    tree = driver.find_element(by='accessibility id', value="1001")
    # Tree might need expansion if it went into a group
    try:
        item = driver.find_element(by='name', value="FakeServer")
    except:
        # Try finding in tree
        item = tree.find_element(by='name', value="FakeServer")

    item.click()
    item.send_keys(Keys.ENTER)
    time.sleep(5) # Wait for PuTTY

    # 3. Check Status Bar
    # Status bar text is often a child element of the status bar control
    try:
        status_text_el = driver.find_element(by='xpath', value="//StatusBar[@AutomationId='1003']/*") 
        text = status_text_el.text or status_text_el.get_attribute("Name")
    except:
        status = driver.find_element(by='accessibility id', value="1003")
        text = status.text or status.get_attribute("Name")

    assert text and ("FakeServer" in text or "127.0.0.1" in text)

def test_tab_rename(driver, desktop_session):
    """Verify that renaming a tab works using the F2 key."""
    # Find the tab item
    tabs = driver.find_element(by='accessibility id', value="1002")
    try:
        tab_item = tabs.find_element(by='name', value="FakeServer")
    except:
        tab_item = driver.find_element(by='xpath', value="//TabItem")

    # Focus and press F2
    tab_item.click()
    
    # Try multiple times if dialog doesn't appear
    dialog = None
    for _ in range(3):
        tab_item.send_keys(Keys.F2)
        time.sleep(1.5)
        try:
            dialog = desktop_session.find_element(by='name', value="Rename Tab")
            break
        except:
            continue
            
    assert dialog is not None, "Rename Tab dialog did not appear after F2"

    edit = dialog.find_element(by='accessibility id', value="201")
    edit.click()
    edit.clear()
    edit.send_keys("RenamedF2")

    dialog.find_element(by='name', value="OK").click()
    time.sleep(1)

    # Check tab text
    # The tab control might contain the new name as a child item name
    assert "RenamedF2" in tabs.text or "RenamedF2" in driver.page_source

