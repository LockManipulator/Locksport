const graph1 = new Chart(document.getElementById("graph1").getContext("2d"), {
  type: "line",
  data: {
    datasets: [
      {
        label: "RCP",
        data: [],
        borderColor: "rgb(255, 50, 50)",
        backgroundColor: "rgb(255, 50, 50)",
        pointStyle: 'circle',
        radius: 3,
        fill: false,
      },
    ],
  },
  options: {
    plugins: {
      legend: { display: true },
    },
    scales: {
      x: {
        type: 'linear',
        display: true,
        position: 'bottom',
        ticks: {
          color: "rgb(202, 202, 202)",
          stepSize: 1, // Adjust the step size to control how often values appear
        },
        grid: {
          borderColor: "rgb(202, 202, 202)"
        }
      },
    },
  }
});

const graph2 = new Chart(document.getElementById("graph2").getContext("2d"), {
  type: "line",
  data: {
    datasets: [
      {
        label: "LCP",
        data: [],
        borderColor: "rgb(50, 50, 255)",
        backgroundColor: "rgb(50, 50, 255)",
        pointStyle: 'circle',
        radius: 3,
        fill: false,
      },
    ],
  },
  options: {
    plugins: {
      legend: { display: true },
    },
    scales: {
      x: {
        type: 'linear',
        display: true,
        position: 'bottom',
        ticks: {
          color: "rgb(202, 202, 202)",
          stepSize: 1, // Adjust the step size to control how often values appear
        },
        grid: {
          borderColor: "rgb(202, 202, 202)"
        }
      },
    },
  }
});

document.getElementById("upnumber").addEventListener("click", function () {
  document.getElementById('testing').value = parseInt(document.getElementById('testing').value) + 2;
});

document.getElementById("downnumber").addEventListener("click", function () {
  document.getElementById('testing').value = parseInt(document.getElementById('testing').value) - 2;
});

document.getElementById("plotrcp").addEventListener("click", function () {
  fetch("/sensor")
    .then((response) => response.text())
    .then((data) => {
      const testing = parseInt(document.getElementById('testing').value);
      const rcontactpoint = parseFloat(data.trim());
      const gdata = graph1.data.datasets[0].data;
      const existingPoint = gdata.find(point => point.x === testing);

      if (existingPoint) {
        existingPoint.y = y;
      } else {
        graph1.data.datasets[0].data.push({x: testing, y: rcontactpoint});
      }

      graph1.update();

      const checkbox = document.querySelector('.switch input');

      if (checkbox.checked) {
        document.getElementById('testing').value = parseInt(document.getElementById('testing').value) + 2;
      }
    });
});

document.getElementById("plotlcp").addEventListener("click", function () {
  fetch("/sensor")
    .then((response) => response.text())
    .then((data) => {
      const testing = parseInt(document.getElementById('testing').value);
      const lcontactpoint = parseFloat(data.trim());
      const gdata = graph2.data.datasets[0].data;
      const existingPoint = gdata.find(point => point.x === testing);

      if (existingPoint) {
        existingPoint.y = y;
      } else {
        graph2.data.datasets[0].data.push({x: testing, y: lcontactpoint});
      }

      graph2.update();

      const checkbox = document.querySelector('.switch input');

      if (!checkbox.checked) {
        document.getElementById('testing').value = parseInt(document.getElementById('testing').value) + 2;
      }
    });
});

document.getElementById("savegraph1").addEventListener("click", function () {
  const image = graph1.toBase64Image();
  const link = document.createElement("a");
  link.href = image;
  link.download = "RCP.png";
  link.click();
});

document.getElementById("savegraph2").addEventListener("click", function () {
  const image = graph2.toBase64Image();
  const link = document.createElement("a");
  link.href = image;
  link.download = "LCP.png";
  link.click();
});

document.getElementById("cleargraphs").addEventListener("click", function () {
  graph1.data.datasets[0].data = [];
  graph2.data.datasets[0].data = [];
  graph1.update();
  graph2.update();
});