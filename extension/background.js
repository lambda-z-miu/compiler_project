const HOST_NAME = "com.compiler_project.my_chem";
const SERVICE_URL = "http://127.0.0.1:17654/render";

async function renderWithHttpService(source) {
  const response = await fetch(SERVICE_URL, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ source }),
  });

  if (!response.ok) {
    throw new Error(`HTTP service returned ${response.status}`);
  }
  return response.json();
}

function renderWithNativeHost(source) {
  return new Promise((resolve) => {
    chrome.runtime.sendNativeMessage(HOST_NAME, { source }, (response) => {
      if (chrome.runtime.lastError) {
        resolve({
          ok: false,
          error: chrome.runtime.lastError.message,
        });
        return;
      }
      resolve(response || { ok: false, error: "Empty native host response" });
    });
  });
}

chrome.runtime.onMessage.addListener((message, _sender, sendResponse) => {
  if (!message || message.type !== "render-my-chem") {
    return false;
  }

  const source = message.source || "";

  renderWithHttpService(source)
    .then(sendResponse)
    .catch(async (httpError) => {
      const nativeResponse = await renderWithNativeHost(source);
      if (nativeResponse.ok) {
        sendResponse(nativeResponse);
        return;
      }
      sendResponse({
        ok: false,
        error: `localhost service failed: ${httpError.message}; native host failed: ${nativeResponse.error}`,
      });
    });

  return true;
});
