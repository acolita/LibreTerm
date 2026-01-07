import pytest
import time
import requests


def find_connection_dialog(driver, desktop_session):
    """Helper to find the Connection Details dialog."""
    try:
        return driver.find_element(by='name', value="Connection Details")
    except Exception:
        return desktop_session.find_element(by='name', value="Connection Details")


def open_new_connection_dialog(driver, desktop_session):
    """Open the New Connection dialog via File menu."""
    file_menu = driver.find_element(by='name', value="File")
    file_menu.click()
    time.sleep(0.5)

    new_conn = desktop_session.find_element(by='name', value="New Connection")
    new_conn.click()
    time.sleep(1)

    return find_connection_dialog(driver, desktop_session)


def get_tree_view(driver):
    """Get the connections tree view."""
    return driver.find_element(by='accessibility id', value="1001")  # IDC_TREEVIEW


def right_click_element(driver, element):
    """
    Perform a right-click on an element using WinAppDriver's legacy mouse API.
    WinAppDriver doesn't support W3C Actions for right-click, so we use direct HTTP calls.
    """
    # Use WinAppDriver's legacy mouse endpoints
    session_url = f"http://127.0.0.1:4723/session/{driver.session_id}"

    # Get the element ID - WinAppDriver uses 'ELEMENT' key
    element_id = element.id

    # Move to element center using the element reference
    requests.post(f"{session_url}/moveto", json={"element": element_id})
    time.sleep(0.1)

    # Right-click (button 2)
    requests.post(f"{session_url}/click", json={"button": 2})


@pytest.mark.usefixtures("clean_connections")
def test_connection_crud(driver, desktop_session):
    """
    Tests the full CRUD (Create, Read, Update, Delete) functionality
    of the Connection Manager.
    """
    # --- 1. CREATE ---
    dialog = open_new_connection_dialog(driver, desktop_session)
    assert dialog is not None, "Connection Details dialog not found"

    # Fill in connection details
    # IDC_EDIT_NAME = 201, IDC_EDIT_HOST = 202, IDC_EDIT_PORT = 203
    # IDC_EDIT_USER = 204, IDC_EDIT_GROUP = 206
    name_input = dialog.find_element(by='accessibility id', value="201")
    host_input = dialog.find_element(by='accessibility id', value="202")
    port_input = dialog.find_element(by='accessibility id', value="203")
    user_input = dialog.find_element(by='accessibility id', value="204")
    group_input = dialog.find_element(by='accessibility id', value="206")

    name_input.clear()
    name_input.send_keys("TestConnection")
    group_input.clear()
    group_input.send_keys("TestGroup")
    host_input.clear()
    host_input.send_keys("192.168.1.100")
    port_input.clear()
    port_input.send_keys("22")
    user_input.clear()
    user_input.send_keys("testuser")

    dialog.find_element(by='name', value="OK").click()
    time.sleep(1)

    # Verify connection appears in tree
    tree = get_tree_view(driver)
    # The connection should appear under TestGroup or as a direct child
    # Try to find the connection by name
    try:
        conn_item = tree.find_element(by='name', value="TestConnection")
    except Exception:
        # It might be under a group, expand and try again
        try:
            group_item = tree.find_element(by='name', value="TestGroup")
            group_item.click()  # Expand
            time.sleep(0.5)
            conn_item = tree.find_element(by='name', value="TestConnection")
        except Exception:
            # Search in desktop session as fallback
            conn_item = desktop_session.find_element(by='name', value="TestConnection")

    assert conn_item is not None, "New connection not found in tree"

    # --- 2. READ (Edit to verify values) ---
    conn_item.click()
    time.sleep(0.3)
    right_click_element(driver, conn_item)
    time.sleep(0.5)

    edit_menu = desktop_session.find_element(by='name', value="Edit")
    edit_menu.click()
    time.sleep(1)

    dialog = find_connection_dialog(driver, desktop_session)

    # Verify the values
    assert dialog.find_element(by='accessibility id', value="201").text == "TestConnection"
    assert dialog.find_element(by='accessibility id', value="202").text == "192.168.1.100"
    assert dialog.find_element(by='accessibility id', value="203").text == "22"
    assert dialog.find_element(by='accessibility id', value="204").text == "testuser"

    dialog.find_element(by='name', value="Cancel").click()
    time.sleep(0.5)

    # --- 3. UPDATE ---
    tree = get_tree_view(driver)
    # Re-find the connection
    try:
        conn_item = tree.find_element(by='name', value="TestConnection")
    except Exception:
        group_item = tree.find_element(by='name', value="TestGroup")
        conn_item = group_item.find_element(by='name', value="TestConnection")

    conn_item.click()
    time.sleep(0.3)
    right_click_element(driver, conn_item)
    time.sleep(0.5)

    edit_menu = desktop_session.find_element(by='name', value="Edit")
    edit_menu.click()
    time.sleep(1)

    dialog = find_connection_dialog(driver, desktop_session)

    # Update the name and host
    name_input = dialog.find_element(by='accessibility id', value="201")
    host_input = dialog.find_element(by='accessibility id', value="202")

    name_input.clear()
    name_input.send_keys("UpdatedConnection")
    host_input.clear()
    host_input.send_keys("10.0.0.50")

    dialog.find_element(by='name', value="OK").click()
    time.sleep(1)

    # Verify the update
    tree = get_tree_view(driver)
    try:
        updated_item = tree.find_element(by='name', value="UpdatedConnection")
    except Exception:
        group_item = tree.find_element(by='name', value="TestGroup")
        updated_item = group_item.find_element(by='name', value="UpdatedConnection")

    assert updated_item is not None, "Updated connection not found in tree"

    # Verify old name is gone
    with pytest.raises(Exception):
        tree.find_element(by='name', value="TestConnection")

    # --- 4. DELETE ---
    updated_item.click()
    time.sleep(0.3)
    right_click_element(driver, updated_item)
    time.sleep(0.5)

    delete_menu = desktop_session.find_element(by='name', value="Delete")
    delete_menu.click()
    time.sleep(1)

    # Handle confirmation dialog if any
    try:
        confirm_dialog = desktop_session.find_element(by='name', value="Confirm Delete")
        confirm_dialog.find_element(by='name', value="Yes").click()
        time.sleep(0.5)
    except Exception:
        pass  # No confirmation dialog

    # Verify it's gone
    tree = get_tree_view(driver)
    with pytest.raises(Exception):
        tree.find_element(by='name', value="UpdatedConnection")


@pytest.mark.usefixtures("clean_connections")
def test_connection_with_group(driver, desktop_session):
    """
    Tests creating connections with groups and verifying tree structure.
    """
    # Create first connection in GroupA
    dialog = open_new_connection_dialog(driver, desktop_session)

    name_input = dialog.find_element(by='accessibility id', value="201")
    group_input = dialog.find_element(by='accessibility id', value="206")
    host_input = dialog.find_element(by='accessibility id', value="202")
    port_input = dialog.find_element(by='accessibility id', value="203")

    name_input.clear()
    name_input.send_keys("Connection1")
    group_input.clear()
    group_input.send_keys("GroupA")
    host_input.clear()
    host_input.send_keys("host1.example.com")
    port_input.clear()
    port_input.send_keys("22")

    dialog.find_element(by='name', value="OK").click()
    time.sleep(1)

    # Create second connection in GroupA
    dialog = open_new_connection_dialog(driver, desktop_session)

    name_input = dialog.find_element(by='accessibility id', value="201")
    group_input = dialog.find_element(by='accessibility id', value="206")
    host_input = dialog.find_element(by='accessibility id', value="202")
    port_input = dialog.find_element(by='accessibility id', value="203")

    name_input.clear()
    name_input.send_keys("Connection2")
    group_input.clear()
    group_input.send_keys("GroupA")
    host_input.clear()
    host_input.send_keys("host2.example.com")
    port_input.clear()
    port_input.send_keys("22")

    dialog.find_element(by='name', value="OK").click()
    time.sleep(1)

    # Verify both connections exist
    tree = get_tree_view(driver)

    # Find GroupA and expand it
    try:
        group_a = tree.find_element(by='name', value="GroupA")
        group_a.click()
        time.sleep(0.5)
    except Exception:
        pass  # Group might already be expanded

    conn1 = tree.find_element(by='name', value="Connection1")
    conn2 = tree.find_element(by='name', value="Connection2")

    assert conn1 is not None, "Connection1 not found"
    assert conn2 is not None, "Connection2 not found"

    # Delete both connections
    for conn_name in ["Connection1", "Connection2"]:
        tree = get_tree_view(driver)
        try:
            conn_item = tree.find_element(by='name', value=conn_name)
        except Exception:
            group_a = tree.find_element(by='name', value="GroupA")
            conn_item = group_a.find_element(by='name', value=conn_name)

        conn_item.click()
        time.sleep(0.3)
        right_click_element(driver, conn_item)
        time.sleep(0.5)

        delete_menu = desktop_session.find_element(by='name', value="Delete")
        delete_menu.click()
        time.sleep(0.5)

        # Handle confirmation if any
        try:
            confirm_dialog = desktop_session.find_element(by='name', value="Confirm Delete")
            confirm_dialog.find_element(by='name', value="Yes").click()
            time.sleep(0.5)
        except Exception:
            pass
