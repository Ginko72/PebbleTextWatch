'use strict';

var DEFAULT_SETTINGS = {
    TimeRenderOh: 1,
    DateSeparatorLine: 0,
    DateOutsideJustified: 1,
    DateFormatUS: 1
};

function getSettings() {
    try {
        return JSON.parse(localStorage.getItem('settings')) || DEFAULT_SETTINGS;
    } catch (e) {
        return DEFAULT_SETTINGS;
    }
}

var CONFIG_HTML = [
    '<!DOCTYPE html><html><head>',
    '<meta name="viewport" content="width=device-width,initial-scale=1">',
    '<style>',
    'body{background:#1a1a1a;color:#eee;font-family:sans-serif;padding:20px;max-width:400px;margin:0 auto}',
    'h2{color:#fff;margin:0 0 20px}',
    '.row{display:flex;justify-content:space-between;align-items:center;',
    'padding:14px 0;border-bottom:1px solid #333}',
    'label{font-size:15px;flex:1;padding-right:12px}',
    'input[type=checkbox]{width:44px;height:28px;cursor:pointer;flex-shrink:0}',
    'button{display:block;width:100%;padding:14px;margin-top:20px;',
    'background:#5b9bd5;color:#fff;border:0;border-radius:6px;font-size:16px;cursor:pointer}',
    '</style></head><body>',
    '<h2>Text Watch</h2>',
    '<div class="row"><label>Render \u201coh\u201d for zero minutes</label><input type="checkbox" id="oh"></div>',
    '<div class="row"><label>Date separator line</label><input type="checkbox" id="sep"></div>',
    '<div class="row"><label>Outside-justified date/day</label><input type="checkbox" id="oj"></div>',
    '<div class="row"><label>US date format (MM.DD)</label><input type="checkbox" id="us"></div>',
    '<button onclick="save()">Save</button>',
    '<script>',
    '(function(){',
    'var s={};',
    'try{s=JSON.parse(decodeURIComponent(location.hash.slice(1)));}catch(e){}',
    'document.getElementById("oh").checked =s.TimeRenderOh      !==0;',
    'document.getElementById("sep").checked=!!s.DateSeparatorLine;',
    'document.getElementById("oj").checked =s.DateOutsideJustified!==0;',
    'document.getElementById("us").checked =s.DateFormatUS       !==0;',
    '})();',
    'function save(){',
    'var c={',
    'TimeRenderOh:          +document.getElementById("oh").checked,',
    'DateSeparatorLine:     +document.getElementById("sep").checked,',
    'DateOutsideJustified:  +document.getElementById("oj").checked,',
    'DateFormatUS:          +document.getElementById("us").checked',
    '};',
    'location.href="pebblejs://close#"+encodeURIComponent(JSON.stringify(c));',
    '}',
    '<\/script></body></html>'
].join('');

Pebble.addEventListener('ready', function () {});

Pebble.addEventListener('showConfiguration', function () {
    var settings = getSettings();
    var url = 'data:text/html,' + encodeURIComponent(CONFIG_HTML) +
              '#' + encodeURIComponent(JSON.stringify(settings));
    Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function (e) {
    if (!e.response || e.response === 'CANCELLED') { return; }
    var config;
    try {
        config = JSON.parse(decodeURIComponent(e.response));
    } catch (ex) { return; }
    localStorage.setItem('settings', JSON.stringify(config));
    Pebble.sendAppMessage(config, function () {}, function (msg) {
        console.log('Config send failed: ' + JSON.stringify(msg));
    });
});
