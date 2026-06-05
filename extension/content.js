const TAG_NAME = "my-chem";
const TAG_SELECTORS = "my-chem,my-Chem,MY-CHEM";
const TEXT_MARKER_RE = /(?:<|&lt;)my-Chem(?:>|&gt;)([\s\S]*?)(?:<|&lt;)\/my-Chem(?:>|&gt;)/gi;
const DOM_TEXT_MARKER_RE = /(?:<|&lt;)my-Chem(?:>|&gt;)([\s\S]*?)(?:<|&lt;)\/my-Chem(?:>|&gt;)/gi;
let pendingRender = false;

function normalizeSource(source) {
  const trimmed = source.trim();
  if (trimmed.length >= 2) {
    const first = trimmed[0];
    const last = trimmed[trimmed.length - 1];
    if ((first === "'" && last === "'") || (first === '"' && last === '"')) {
      return trimmed.slice(1, -1).trim();
    }
  }
  return trimmed;
}

function parseSvg(svgText) {
  const doc = new DOMParser().parseFromString(svgText, "image/svg+xml");
  const svg = doc.documentElement;
  if (!svg || svg.localName.toLowerCase() !== "svg") {
    throw new Error("Native host did not return an SVG document");
  }
  return document.importNode(svg, true);
}

function makeOriginalTextNode(originalText) {
  const span = document.createElement("span");
  span.dataset.myChemRendered = "1";
  span.textContent = originalText;
  return span;
}

function isChemElement(node) {
  return Boolean(node && node.nodeType === Node.ELEMENT_NODE && node.localName && node.localName.toLowerCase() === TAG_NAME);
}

function containsChemMarker(node) {
  if (!node) {
    return false;
  }
  const text = node.nodeType === Node.TEXT_NODE ? node.nodeValue : node.textContent;
  return Boolean(text && text.toLowerCase().includes("my-chem"));
}

function shouldSkipTextNode(node) {
  if (!node.nodeValue) {
    return true;
  }
  const parent = node.parentElement;
  if (!parent) {
    return true;
  }
  const tag = parent.localName ? parent.localName.toLowerCase() : "";
  if (tag === "script" || tag === "style" || tag === "textarea" || tag === "svg") {
    return true;
  }
  return Boolean(parent.closest("[data-my-chem-rendered='1'],svg"));
}

async function renderElement(element) {
  if (element.dataset.myChemRendered === "1") {
    return;
  }

  const source = normalizeSource(element.textContent);
  if (!source) {
    return;
  }
  element.dataset.myChemRendered = "1";

  let response;
  try {
    response = await chrome.runtime.sendMessage({
      type: "render-my-chem",
      source,
    });
  } catch (error) {
    return;
  }

  if (!response || !response.ok) {
    return;
  }

  try {
    const svg = parseSvg(response.svg);
    svg.setAttribute("data-my-chem-source", source);
    element.replaceWith(svg);
  } catch (error) {
    return;
  }
}

function insertRenderedPlaceholder(source, placeholder, originalText) {
  renderSourceToNode(source).then((node) => {
    if (node) {
      placeholder.replaceWith(node);
    } else {
      placeholder.replaceWith(makeOriginalTextNode(originalText));
    }
  });
}

async function renderSourceToNode(source) {
  let response;
  try {
    response = await chrome.runtime.sendMessage({
      type: "render-my-chem",
      source,
    });
  } catch (error) {
    return null;
  }

  if (!response || !response.ok) {
    return null;
  }

  try {
    const svg = parseSvg(response.svg);
    svg.setAttribute("data-my-chem-source", source);
    return svg;
  } catch (error) {
    return null;
  }
}

function renderTextNode(textNode) {
  if (shouldSkipTextNode(textNode)) {
    return;
  }
  const text = textNode.nodeValue;
  if (!text || !TEXT_MARKER_RE.test(text)) {
    TEXT_MARKER_RE.lastIndex = 0;
    return;
  }
  TEXT_MARKER_RE.lastIndex = 0;

  const fragment = document.createDocumentFragment();
  let lastIndex = 0;
  let match;
  while ((match = TEXT_MARKER_RE.exec(text)) !== null) {
    if (match.index > lastIndex) {
      fragment.append(document.createTextNode(text.slice(lastIndex, match.index)));
    }

    const originalText = match[0];
    const source = normalizeSource(match[1]);
    if (!source) {
      fragment.append(makeOriginalTextNode(originalText));
      lastIndex = match.index + match[0].length;
      continue;
    }

    const placeholder = document.createElement("span");
    placeholder.textContent = "Rendering my-Chem...";
    placeholder.dataset.myChemRendered = "1";
    fragment.append(placeholder);
    insertRenderedPlaceholder(source, placeholder, originalText);

    lastIndex = match.index + match[0].length;
  }

  if (lastIndex < text.length) {
    fragment.append(document.createTextNode(text.slice(lastIndex)));
  }
  textNode.replaceWith(fragment);
}

function renderTextMarkers(root = document.body) {
  if (!root) {
    return;
  }
  const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT, {
    acceptNode(node) {
      if (shouldSkipTextNode(node) || !node.nodeValue.toLowerCase().includes("my-chem")) {
        return NodeFilter.FILTER_REJECT;
      }
      return NodeFilter.FILTER_ACCEPT;
    },
  });

  const nodes = [];
  while (walker.nextNode()) {
    nodes.push(walker.currentNode);
  }
  for (const node of nodes) {
    renderTextNode(node);
  }
}

function collectTextSegments(root) {
  const segments = [];
  let text = "";
  const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT, {
    acceptNode(node) {
      return shouldSkipTextNode(node) ? NodeFilter.FILTER_REJECT : NodeFilter.FILTER_ACCEPT;
    },
  });

  while (walker.nextNode()) {
    const nodeText = walker.currentNode.nodeValue || "";
    if (!nodeText) {
      continue;
    }
    const start = text.length;
    text += nodeText;
    segments.push({
      node: walker.currentNode,
      start,
      end: text.length,
    });
  }
  return { text, segments };
}

function findTextPosition(segments, absoluteOffset, preferEnd = false) {
  for (const segment of segments) {
    if (absoluteOffset >= segment.start && absoluteOffset < segment.end) {
      return {
        node: segment.node,
        offset: absoluteOffset - segment.start,
      };
    }
    if (preferEnd && absoluteOffset === segment.end) {
      return {
        node: segment.node,
        offset: segment.node.nodeValue.length,
      };
    }
  }
  return null;
}

function renderDomTextMarkerRanges(root = document.body) {
  if (!root) {
    return;
  }

  const { text, segments } = collectTextSegments(root);
  if (!text.toLowerCase().includes("my-chem")) {
    return;
  }

  DOM_TEXT_MARKER_RE.lastIndex = 0;
  const matches = [];
  let match;
  while ((match = DOM_TEXT_MARKER_RE.exec(text)) !== null) {
    matches.push({
      start: match.index,
      end: match.index + match[0].length,
      originalText: match[0],
      source: normalizeSource(match[1]),
    });
  }

  for (const item of matches.reverse()) {
    const start = findTextPosition(segments, item.start);
    const end = findTextPosition(segments, item.end, true);
    if (!start || !end || !item.source) {
      continue;
    }

    const range = document.createRange();
    range.setStart(start.node, start.offset);
    range.setEnd(end.node, end.offset);

    const placeholder = document.createElement("span");
    placeholder.textContent = "Rendering my-Chem...";
    placeholder.dataset.myChemRendered = "1";
    range.deleteContents();
    range.insertNode(placeholder);
    insertRenderedPlaceholder(item.source, placeholder, item.originalText);
  }
}

function renderAll(root = document) {
  if (!root) {
    return;
  }
  if (isChemElement(root)) {
    renderElement(root);
  }
  if (typeof root.querySelectorAll === "function") {
    const elements = Array.from(root.querySelectorAll(TAG_SELECTORS));
    for (const element of elements) {
      renderElement(element);
    }
  }
  if (root === document || root.nodeType === Node.ELEMENT_NODE || root.nodeType === Node.DOCUMENT_FRAGMENT_NODE) {
    const textRoot = root === document ? document.body : root;
    renderTextMarkers(textRoot);
    renderDomTextMarkerRanges(textRoot);
  }
}

function scheduleRender() {
  if (pendingRender) {
    return;
  }
  pendingRender = true;
  setTimeout(() => {
    pendingRender = false;
    renderAll(document.body);
  }, 150);
}

renderAll();

const observer = new MutationObserver((mutations) => {
  for (const mutation of mutations) {
    if (mutation.type === "characterData") {
      renderTextNode(mutation.target);
      if (containsChemMarker(mutation.target)) {
        scheduleRender();
      }
      continue;
    }

    for (const node of mutation.addedNodes) {
      if (node.nodeType === Node.TEXT_NODE) {
        renderTextNode(node);
        if (containsChemMarker(node)) {
          scheduleRender();
        }
        continue;
      }
      if (node.nodeType !== Node.ELEMENT_NODE) {
        continue;
      }
      if (isChemElement(node)) {
        renderElement(node);
      }
      renderAll(node);
      if (containsChemMarker(node)) {
        scheduleRender();
      }
    }
  }
});

observer.observe(document.documentElement, {
  childList: true,
  characterData: true,
  subtree: true,
});
