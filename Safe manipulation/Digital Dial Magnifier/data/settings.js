document.getElementById("changeBackground").addEventListener("click", function () {
  const r = document.getElementById("r").value;
  const g = document.getElementById("g").value;
  const b = document.getElementById("b").value;
  fetch(`/bgcolor?r=${r}&g=${g}&b=${b}`);
});

document.getElementById("changeForeground").addEventListener("click", function () {
  const r = document.getElementById("r").value;
  const g = document.getElementById("g").value;
  const b = document.getElementById("b").value;
  fetch(`/fgcolor?r=${r}&g=${g}&b=${b}`);
});

document.getElementById("changeBorder").addEventListener("click", function () {
  const r = document.getElementById("r").value;
  const g = document.getElementById("g").value;
  const b = document.getElementById("b").value;
  fetch(`/bordercolor?r=${r}&g=${g}&b=${b}`);
});

document.getElementById("save").addEventListener("click", function () {
  const screen_rotation = document.getElementById("screen_rotation").value;
  const text_size = document.getElementById("text_size").value;
  const boot_screen = document.getElementById("boot_screen").value;
  const boot_image = document.getElementById("boot_image").value;
  const boot_time = document.getElementById("boot_time").value;
  const dial_border = document.getElementById("dial_border").value;
  const dial_border_thickness = document.getElementById("dial_border_thickness").value;
  const show_pointer = document.getElementById("show_pointer").value;
  const pointer_size = document.getElementById("pointer_size").value;
  const update_interval = document.getElementById("update_interval").value;
  const ssid = document.getElementById("ssid").value;
  const wifi_pass = document.getElementById("wifi_pass").value;
  const web_username = document.getElementById("web_username").value;
  const web_password = document.getElementById("web_password").value;
  fetch(`/savesettings?screen_rotation=${screen_rotation}&text_size=${text_size}&boot_screen=${boot_screen}&boot_image=${boot_image}&boot_time=${boot_time}&dial_border=${dial_border}&dial_border_thickness=${dial_border_thickness}&show_pointer=${show_pointer}&pointer_size=${pointer_size}&update_interval=${update_interval}&ssid=${ssid}&wifi_pass=${wifi_pass}&web_username=${web_username}&web_password=${web_password}`);
});

// Function to fetch and update default options
function fetchAndSetDefaults() {
    fetch('/getoptions')
        .then(response => response.json())
        .then(data => {
            document.getElementById('text_size').placeholder = data.text_size;
            document.getElementById('boot_time').placeholder = data.boot_time;
            document.getElementById('dial_border_thickness').placeholder = data.border_thickness;
            document.getElementById('pointer_size').placeholder = data.pointer_size;
            document.getElementById('update_interval').placeholder = data.update_interval;
            document.getElementById('ssid').placeholder = data.wifi_ssid;
            document.getElementById('wifi_pass').placeholder = data.wifi_password;
            document.getElementById('web_username').placeholder = data.webpage_username;
            document.getElementById('web_password').placeholder = data.webpage_password;
            document.getElementById('text_size').value = data.text_size;
            document.getElementById('boot_time').value = data.boot_time;
            document.getElementById('dial_border_thickness').value = data.border_thickness;
            document.getElementById('pointer_size').value = data.pointer_size;
            document.getElementById('update_interval').value = data.update_interval;
            document.getElementById('ssid').value = data.wifi_ssid;
            document.getElementById('wifi_pass').value = data.wifi_password;
            document.getElementById('web_username').value = data.webpage_username;
            document.getElementById('web_password').value = data.webpage_password;
            document.getElementById('screen_rotation').value = data.screen_rotation;
            document.getElementById('screen_rotation').value = data.screen_rotation;
            document.getElementById('boot_image').value = data.boot_image;
            document.getElementById('dial_border').value = data.dial_border;
            document.getElementById('show_pointer').value = data.show_pointer;
        })
        .catch(error => console.error('Error fetching options:', error));
}

// Function to dynamically update the default options
function updateSelectOptions() {
    fetch('/getBoots')
        .then(response => response.json())
        .then(data => {
            const selectElement = document.getElementById("boot_image");
            selectElement.innerHTML = ""; // Clear existing options
            data.forEach(value => {
                const option = document.createElement("option");
                option.value = value;
                option.textContent = value;
                selectElement.appendChild(option);
            });
            selectElement.size = options.length; // Adjust size once options are added
        })
        .catch(error => console.error('Error fetching options:', error));
}

// Run the function once on page load
window.onload = function () {
    updateSelectOptions();
    fetchAndSetDefaults();
};