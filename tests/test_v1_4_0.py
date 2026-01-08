import pytest
import time
from selenium.webdriver.common.keys import Keys

def test_settings_plink_path(driver, desktop_session):
    """Verify Plink Path can be set in Settings."""
    
    file_menu = driver.find_element(by='name', value="File")
    file_menu.click()
    time.sleep(0.5)
    
    try:
        settings_item = desktop_session.find_element(by='name', value="Settings")
        settings_item.click()
    except:
        file_menu.send_keys('s') # Mnemonic
        
    time.sleep(1)
    
    dialog = desktop_session.find_element(by='name', value="Settings")
    
    # IDC_EDIT_PLINK_PATH = 215
    plink_edit = dialog.find_element(by='accessibility id', value="215")
    plink_edit.click()
    plink_edit.clear()
    plink_edit.send_keys("C:\\FakePath\\plink.exe")
    
    dialog.find_element(by='name', value="OK").click()
    time.sleep(0.5)
    
    # Re-open to verify save
    file_menu = driver.find_element(by='name', value="File")
    file_menu.click()
    time.sleep(0.5)
    try:
        settings_item = desktop_session.find_element(by='name', value="Settings")
        settings_item.click()
    except:
        file_menu.send_keys('s')
    
    dialog = desktop_session.find_element(by='name', value="Settings")
    plink_edit = dialog.find_element(by='accessibility id', value="215")
    assert "C:\\FakePath\\plink.exe" in plink_edit.text
    
    dialog.find_element(by='name', value="Cancel").click()

def test_jump_server_ui(driver, desktop_session):
    """Verify Jump Server combo box exists and functions."""
    
    # 1. Create Connection A (Jump Host)
    file_menu = driver.find_element(by='name', value="File")
    file_menu.click()
    try:
        new_conn = desktop_session.find_element(by='name', value="New Connection")
        new_conn.click()
    except:
        file_menu.send_keys('n')
        
    dialog = desktop_session.find_element(by='name', value="Connection Details")
    dialog.find_element(by='accessibility id', value="201").send_keys("MyJumpHost")
    dialog.find_element(by='name', value="OK").click()
    time.sleep(1)
    
    # 2. Create Connection B (Target)
    file_menu = driver.find_element(by='name', value="File")
    file_menu.click()
    try:
        new_conn = desktop_session.find_element(by='name', value="New Connection")
        new_conn.click()
    except:
        file_menu.send_keys('n')
        
    dialog = desktop_session.find_element(by='name', value="Connection Details")
    dialog.find_element(by='accessibility id', value="201").send_keys("TargetHost")
    
    # Check Jump Combo (IDC_COMBO_JUMP_SERVER = 208)
    jump_combo = dialog.find_element(by='accessibility id', value="208")
    jump_combo.click()
    # Select MyJumpHost
    # WinAppDriver combo box selection can be tricky. Sending down keys might work.
    jump_combo.send_keys(Keys.ARROW_DOWN) # Assuming <None> is first, MyJumpHost is second
    jump_combo.send_keys(Keys.ENTER)
    
    dialog.find_element(by='name', value="OK").click()
    time.sleep(1)
    
    # 3. Verify Selection Saved
    # Find TargetHost in tree
    tree = driver.find_element(by='accessibility id', value="1001")
    try:
        item = tree.find_element(by='name', value="TargetHost")
    except:
        item = driver.find_element(by='name', value="TargetHost")
        
    item.click()
    item.send_keys(Keys.SHIFT + Keys.F10) # Context menu
    time.sleep(0.5)
    
    try:
        edit_menu = desktop_session.find_element(by='name', value="Edit")
        edit_menu.click()
    except:
        # Retry or use mnemonic
        pass
        
    dialog = desktop_session.find_element(by='name', value="Connection Details")
    jump_combo = dialog.find_element(by='accessibility id', value="208")
    # Verify text contains MyJumpHost
    assert "MyJumpHost" in jump_combo.text
    
    dialog.find_element(by='name', value="Cancel").click()
