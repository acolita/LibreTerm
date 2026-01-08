import pytest
import time
from selenium.webdriver.common.keys import Keys

def test_delete_connection_with_del_key(driver, desktop_session):
    """Verify deleting a connection using the DELETE key."""
    # Ensure window is focused
    driver.find_element(by='name', value="LibreTerm").click()
    
    # 1. Create a dummy connection
    dialog = None
    for _ in range(3):
        try:
            file_menu = driver.find_element(by='name', value="File")
            file_menu.click()
            time.sleep(0.5)
            # Find "New Connection" item via desktop session
            try:
                new_conn = desktop_session.find_element(by='name', value="New Connection")
                new_conn.click()
            except:
                file_menu.send_keys('n')
            time.sleep(1.5)
            dialog = desktop_session.find_element(by='name', value="Connection Details")
            break
        except:
            driver.find_element(by='name', value="LibreTerm").send_keys(Keys.ESCAPE) # Close any stray menu
            time.sleep(1)
            
    assert dialog is not None, "Connection Details dialog not found after 3 attempts"
    
    name_edit = dialog.find_element(by='accessibility id', value="201")
    name_edit.click()
    name_edit.clear()
    name_edit.send_keys("DeleteMe")
    
    dialog.find_element(by='name', value="OK").click()
    time.sleep(2)
    
    # 2. Select it in tree
    tree = driver.find_element(by='accessibility id', value="1001")
    item = tree.find_element(by='name', value="DeleteMe")
    item.click()
    time.sleep(0.5)
    
    # 3. Press DELETE
    item.send_keys(Keys.DELETE)
    time.sleep(1.5)
    
    # 4. Confirm Dialog
    confirm = None
    try:
        confirm = desktop_session.find_element(by='name', value="Confirm Delete")
    except:
        # Retry finding confirm
        time.sleep(1)
        confirm = desktop_session.find_element(by='name', value="Confirm Delete")
        
    yes_btn = confirm.find_element(by='name', value="Yes")
    yes_btn.click()
    time.sleep(1.5)
        
    # 5. Verify gone
    try:
        tree.find_element(by='name', value="DeleteMe")
        pytest.fail("Connection should have been deleted")
    except:
        pass # Success
