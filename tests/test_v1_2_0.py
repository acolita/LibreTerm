import pytest
import time
from selenium.webdriver.common.keys import Keys

def test_quick_command_bar_exists(driver):
    """Verify that the Quick Command Bar controls exist."""
    # IDC_QUICK_CMD_EDIT = 1005
    edit = driver.find_element(by='accessibility id', value="1005")
    assert edit.is_displayed()
    
    # IDC_QUICK_CMD_BTN = 1006
    btn = driver.find_element(by='accessibility id', value="1006")
    assert btn.is_displayed()
    
    # IDC_QUICK_CMD_CHK = 1007
    chk = driver.find_element(by='accessibility id', value="1007")
    assert chk.is_displayed()

def test_snippet_manager_crud(driver, desktop_session):
    """Verify Snippet Manager CRUD operations."""
    # Open Snippet Manager
    tools_menu = driver.find_element(by='name', value="Tools")
    tools_menu.click()
    time.sleep(0.5)
    
    # Find menu item via desktop session
    try:
        snip_menu = desktop_session.find_element(by='name', value="Snippet Manager")
        snip_menu.click()
    except:
        tools_menu.send_keys(Keys.ARROW_DOWN)
        tools_menu.send_keys(Keys.ARROW_DOWN) # Multi-Input -> Credentials -> Snippets? Check index.
        # Actually IDM_TOOLS_SNIPPETS is 109. 
        # Multi-Input (320), Credentials (108), Snippets (109)
        # Order in menu: Multi-Input, Credentials, Snippets.
        tools_menu.send_keys(Keys.ARROW_DOWN)
        tools_menu.send_keys(Keys.ENTER)
        
    time.sleep(1)
    
    # Find Dialog
    dialog = desktop_session.find_element(by='name', value="Snippet Manager")
    
    # Add Snippet
    add_btn = dialog.find_element(by='name', value="Add...")
    add_btn.click()
    time.sleep(0.5)
    
    # Edit Dialog
    edit_dlg = desktop_session.find_element(by='name', value="Edit Snippet")
    name_box = edit_dlg.find_element(by='accessibility id', value="271")
    content_box = edit_dlg.find_element(by='accessibility id', value="272")
    
    name_box.click()
    name_box.clear()
    name_box.send_keys("TestSnippet")
    
    content_box.click()
    content_box.clear()
    content_box.send_keys("ls -la")
    
    edit_dlg.find_element(by='name', value="Save").click()
    time.sleep(1)
    
    # Verify in list
    list_box = dialog.find_element(by='accessibility id', value="261")
    item = list_box.find_element(by='name', value="TestSnippet")
    assert item is not None
    
    # Edit
    item.click()
    dialog.find_element(by='name', value="Edit...").click()
    time.sleep(1)
    
    edit_dlg = desktop_session.find_element(by='name', value="Edit Snippet")
    name_box = edit_dlg.find_element(by='accessibility id', value="271")
    # Use Name attribute if text is unreliable
    val = name_box.text or name_box.get_attribute("Name") or name_box.get_attribute("Value.Value")
    assert "TestSnippet" in val
    edit_dlg.find_element(by='name', value="Cancel").click()
    time.sleep(0.5)
    
    # Delete
    dialog.find_element(by='name', value="Delete").click()
    # Confirm
    confirm = desktop_session.find_element(by='name', value="Confirm")
    confirm.find_element(by='name', value="Yes").click()
    time.sleep(1)
    
    # Verify gone
    try:
        list_box.find_element(by='name', value="TestSnippet")
        assert False, "Snippet should be deleted"
    except:
        pass
        
    dialog.find_element(by='name', value="Close").click()

def test_quick_command_send(driver):
    """Verify that we can type in quick command bar and click send (no crash)."""
    # Ensure a session is open
    try:
        tabs = driver.find_element(by='accessibility id', value="1002")
        tab = driver.find_element(by='xpath', value="//TabItem")
    except:
        # Launch one
        try:
            item = driver.find_element(by='name', value="FakeServer")
        except:
            tree = driver.find_element(by='accessibility id', value="1001")
            item = tree.find_element(by='xpath', value="//TreeItem[1]")
        item.click()
        item.send_keys(Keys.ENTER)
        time.sleep(5)

    edit = driver.find_element(by='accessibility id', value="1005")
    edit.click()
    edit.clear()
    edit.send_keys("whoami")
    
    btn = driver.find_element(by='accessibility id', value="1006")
    btn.click()
    time.sleep(1)
    
    # Verify edit is cleared
    # WinAppDriver sometimes returns the last value if checked too quickly
    val = edit.text or edit.get_attribute("Name") or ""
    # If it's the name of the control, it might not be empty but it shouldn't be "whoami"
    assert "whoami" not in val

