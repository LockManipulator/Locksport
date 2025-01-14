setInterval(function () {
  fetch("/sensor")
    .then((response) => response.text())
    .then((data) => {
      document.getElementById("position").innerText = data;
    });
}, 50);