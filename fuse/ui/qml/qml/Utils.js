.pragma library

function shortcutText(text)
{
    var ampersantPos = text.indexOf("&");
    if (ampersantPos === -1)
        return text;
    return text.substr(0, ampersantPos) + "<b><u>" + text.substr(ampersantPos + 1, 1) + "</u></b>" + text.substr(ampersantPos + 2);
}
