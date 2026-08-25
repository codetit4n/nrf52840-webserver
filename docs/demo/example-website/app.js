(function () {
  "use strict";

  var startedAt = Date.now();
  var uptime = document.getElementById("uptime");
  var localTime = document.getElementById("local-time");
  var testButton = document.getElementById("test-file");
  var result = document.getElementById("file-result");
  var content = document.getElementById("file-content");
  var closeButton = document.getElementById("close-result");

  function pad(value) {
    return String(value).padStart(2, "0");
  }

  function updateClocks() {
    var seconds = Math.floor((Date.now() - startedAt) / 1000);
    var now = new Date();

    uptime.textContent = pad(Math.floor(seconds / 3600)) + ":" +
      pad(Math.floor((seconds % 3600) / 60)) + ":" + pad(seconds % 60);
    localTime.textContent = pad(now.getHours()) + ":" + pad(now.getMinutes()) + ":" + pad(now.getSeconds());
  }

  testButton.addEventListener("click", function () {
    testButton.disabled = true;
    testButton.textContent = "Requesting...";
    result.hidden = false;
    content.textContent = "GET /test.txt ...";

    fetch("test.txt", { cache: "no-store" })
      .then(function (response) {
        if (!response.ok) throw new Error("HTTP " + response.status);
        return response.text();
      })
      .then(function (text) {
        content.textContent = text;
      })
      .catch(function (error) {
        content.textContent = "Request failed: " + error.message;
      })
      .finally(function () {
        testButton.disabled = false;
        testButton.textContent = "Request test.txt";
        result.scrollIntoView({ behavior: "smooth", block: "nearest" });
      });
  });

  closeButton.addEventListener("click", function () {
    result.hidden = true;
  });

  updateClocks();
  setInterval(updateClocks, 1000);
}());
