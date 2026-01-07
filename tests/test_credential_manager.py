import pytest
import time


def find_cred_dialog(driver, desktop_session):
    """Helper to find the Credential Manager dialog."""
    try:
        return driver.find_element(by='name', value="Credential Manager")
    except Exception:
        return desktop_session.find_element(by='name', value="Credential Manager")


def find_edit_dialog(driver, desktop_session):
    """Helper to find the Edit Credential dialog."""
    try:
        return driver.find_element(by='name', value="Edit Credential")
    except Exception:
        return desktop_session.find_element(by='name', value="Edit Credential")


@pytest.mark.usefixtures("clean_credentials")
def test_credential_manager_crud(driver, desktop_session):
    """
    Tests the full CRUD (Create, Read, Update, Delete) functionality
    of the Credential Manager.
    """
    # --- 1. Open Credential Manager ---
    tools_menu = driver.find_element(by='name', value="Tools")
    tools_menu.click()
    time.sleep(0.5)

    # Using desktop session to access the menu popup
    cred_manager_item = desktop_session.find_element(by='name', value="Credential Manager")
    cred_manager_item.click()
    time.sleep(1)

    cred_dialog = find_cred_dialog(driver, desktop_session)
    assert cred_dialog is not None, "Credential Manager dialog not found"

    # --- 2. CREATE ---
    cred_dialog.find_element(by='accessibility id', value="232").click()  # Add button (IDC_BTN_CRED_ADD)
    time.sleep(1)

    edit_dialog = find_edit_dialog(driver, desktop_session)
    assert edit_dialog is not None, "Edit Credential (add) dialog not found"

    alias_input = edit_dialog.find_element(by='accessibility id', value="241")  # IDC_EDIT_CRED_ALIAS
    user_input = edit_dialog.find_element(by='accessibility id', value="242")   # IDC_EDIT_CRED_USER
    pass_input = edit_dialog.find_element(by='accessibility id', value="243")   # IDC_EDIT_CRED_PASS

    alias_input.send_keys("TestAlias")
    user_input.send_keys("TestUser")
    pass_input.send_keys("TestPass")

    edit_dialog.find_element(by='name', value="Save").click()
    time.sleep(1)

    # Re-find the dialog and list after Save closes the edit dialog
    cred_dialog = find_cred_dialog(driver, desktop_session)
    cred_list = cred_dialog.find_element(by='accessibility id', value="231")  # IDC_LIST_CREDENTIALS
    new_item = cred_list.find_element(by='name', value="TestAlias")
    assert new_item is not None, "New credential not found in list"

    # --- 3. READ ---
    new_item.click()
    cred_dialog.find_element(by='accessibility id', value="233").click()  # Edit button (IDC_BTN_CRED_EDIT)
    time.sleep(1)

    edit_dialog = find_edit_dialog(driver, desktop_session)

    assert edit_dialog.find_element(by='accessibility id', value="242").text == "TestUser"
    # Note: Password field value cannot be read due to ES_PASSWORD style

    edit_dialog.find_element(by='name', value="Cancel").click()
    time.sleep(0.5)

    # --- 4. UPDATE ---
    # Re-find cred_dialog after cancel
    cred_dialog = find_cred_dialog(driver, desktop_session)
    cred_list = cred_dialog.find_element(by='accessibility id', value="231")
    cred_list.find_element(by='name', value="TestAlias").click()
    cred_dialog.find_element(by='accessibility id', value="233").click()  # Edit button
    time.sleep(1)

    edit_dialog = find_edit_dialog(driver, desktop_session)

    alias_input = edit_dialog.find_element(by='accessibility id', value="241")
    user_input = edit_dialog.find_element(by='accessibility id', value="242")

    alias_input.clear()
    alias_input.send_keys("UpdatedAlias")
    user_input.clear()
    user_input.send_keys("UpdatedUser")

    edit_dialog.find_element(by='name', value="Save").click()
    time.sleep(1)

    # Re-find after Save
    cred_dialog = find_cred_dialog(driver, desktop_session)
    cred_list = cred_dialog.find_element(by='accessibility id', value="231")

    # Verify updated item is in the list
    updated_item = cred_list.find_element(by='name', value="UpdatedAlias")
    assert updated_item is not None, "Updated credential not found in list"

    # Verify old item is gone
    with pytest.raises(Exception):
        cred_list.find_element(by='name', value="TestAlias")

    # --- 5. DELETE ---
    updated_item.click()
    cred_dialog.find_element(by='accessibility id', value="234").click()  # Delete button (IDC_BTN_CRED_DELETE)
    time.sleep(0.5)

    # Re-find after delete
    cred_dialog = find_cred_dialog(driver, desktop_session)
    cred_list = cred_dialog.find_element(by='accessibility id', value="231")

    # Verify it's gone
    with pytest.raises(Exception):
        cred_list.find_element(by='name', value="UpdatedAlias")

    # --- 6. CLEANUP ---
    cred_dialog.find_element(by='name', value="Close").click()
