setInterval(function () {
  fetch("/sensor")
    .then((response) => response.text())
    .then((data) => {
      document.getElementById("position").innerText = data;
    });
}, 50);

document.getElementById("reset").addEventListener("click", function () {
  fetch("/reset")
    .then(() => {
    });
});