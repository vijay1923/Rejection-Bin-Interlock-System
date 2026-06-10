// web_server.h - Modern web portal for SPIFFS data
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFi.h>
#include <WebServer.h>
#include "config.h"
#include "FS.h"
#include "SPIFFS.h"

#define AP_SSID     "RejectionBin_AP"
#define AP_PASSWORD "rejectionbin"
#define AP_IP       IPAddress(192, 168, 1, 21)
#define AP_GATEWAY  IPAddress(192, 168, 1, 21)
#define AP_SUBNET   IPAddress(255, 255, 255, 0)

WebServer server(80);

// ─── Path validation (prevents traversal attacks) ───────────────────────────
bool isValidPath(const String& path) {
    return path.startsWith("/") &&
           path.indexOf("..") == -1 &&
           path.length() > 1 &&
           path.length() < 64;
}

// ─── CSS ─────────────────────────────────────────────────────────────────────
void handleCSS() {
    server.sendHeader("Cache-Control", "max-age=3600");
    server.send(200, "text/css", R"css(
*{box-sizing:border-box;margin:0;padding:0}
:root{
  --bg:#080b12;--bg2:#0d1120;--bg3:#111827;--bg4:#161e30;
  --border:#1c2236;--border2:#232d42;--border3:#2e3a54;
  --text:#dde3f5;--text2:#6b7694;--text3:#3d4660;
  --green:#22c55e;--green-dim:#0a1f12;--green-t:#4ade80;
  --blue:#3b82f6;--blue-dim:#0a1428;--blue-t:#60a5fa;
  --purple:#a855f7;--purple-dim:#150d26;--purple-t:#c084fc;
  --amber:#f59e0b;--amber-dim:#201509;--amber-t:#fbbf24;
  --red:#ef4444;--red-dim:#1f0909;--red-t:#f87171;
  --mono:'JetBrains Mono',Consolas,monospace;
  --sans:'Inter',system-ui,sans-serif;
  --r:8px;--rl:12px;
}
body{font-family:var(--sans);background:var(--bg);color:var(--text);font-size:14px;line-height:1.5;min-height:100vh}

/* ── topbar ── */
.topbar{background:var(--bg2);border-bottom:1px solid var(--border);padding:0 24px;display:flex;align-items:center;justify-content:space-between;height:56px;position:sticky;top:0;z-index:100}
.brand{display:flex;align-items:center;gap:12px}
.logo{width:34px;height:34px;background:linear-gradient(135deg,var(--green-dim),rgba(34,197,94,.18));border:1px solid rgba(34,197,94,.3);border-radius:9px;display:flex;align-items:center;justify-content:center;flex-shrink:0}
.logo svg{width:16px;height:16px;stroke:var(--green);fill:none;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}
.brand-name{font-size:14px;font-weight:700;color:var(--text);letter-spacing:-.01em}
.brand-sub{font-size:11px;color:var(--text3);font-family:var(--mono);margin-top:1px}
.topbar-right{display:flex;align-items:center;gap:10px}
.pill-online{display:flex;align-items:center;gap:7px;padding:5px 12px;background:var(--green-dim);border:1px solid rgba(34,197,94,.2);border-radius:20px}
.pulse{position:relative;width:8px;height:8px;flex-shrink:0}
.pulse::before{content:'';position:absolute;inset:0;border-radius:50%;background:var(--green);animation:pls 2s infinite}
.pulse::after{content:'';position:absolute;inset:-3px;border-radius:50%;border:1.5px solid var(--green);opacity:.4;animation:plsr 2s infinite}
@keyframes pls{0%,100%{opacity:1}50%{opacity:.5}}
@keyframes plsr{0%{transform:scale(1);opacity:.4}100%{transform:scale(1.9);opacity:0}}
.pill-label{font-size:11px;font-weight:700;color:var(--green-t);letter-spacing:.05em}

/* ── nav ── */
.nav{background:var(--bg2);border-bottom:1px solid var(--border);display:flex;padding:0 24px;gap:0}
.nav a{padding:11px 16px;font-size:13px;color:var(--text3);text-decoration:none;border-bottom:2px solid transparent;display:inline-flex;align-items:center;gap:7px;transition:color .15s;font-weight:500}
.nav a:hover{color:var(--text2)}
.nav a.active{color:var(--text);border-bottom-color:var(--green)}
.ni{width:13px;height:13px;stroke:currentColor;fill:none;stroke-width:2;stroke-linecap:round;stroke-linejoin:round;flex-shrink:0}

/* ── layout ── */
.content{padding:24px;max-width:960px;margin:0 auto}
.g4{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:10px;margin-bottom:14px}
.g2{display:grid;grid-template-columns:1fr 1fr;gap:14px;margin-bottom:14px}
.g1{margin-bottom:14px}
@media(max-width:700px){.g4{grid-template-columns:repeat(2,1fr)}.g2{grid-template-columns:1fr}}

/* ── card ── */
.card{background:var(--bg2);border:1px solid var(--border);border-radius:var(--rl);padding:16px 18px}
.card-hd{display:flex;align-items:center;justify-content:space-between;margin-bottom:14px}
.card-title{font-size:11px;font-weight:700;color:var(--text3);text-transform:uppercase;letter-spacing:.09em}

/* ── metric tiles ── */
.tile{background:var(--bg3);border:1px solid var(--border);border-radius:var(--r);padding:14px 16px;transition:border-color .2s}
.tile:hover{border-color:var(--border3)}
.ticon{width:30px;height:30px;border-radius:7px;display:flex;align-items:center;justify-content:center;margin-bottom:10px;flex-shrink:0}
.ticon svg{width:14px;height:14px;stroke:currentColor;fill:none;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}
.ticon.g{background:var(--green-dim);color:var(--green)}
.ticon.b{background:var(--blue-dim);color:var(--blue)}
.ticon.p{background:var(--purple-dim);color:var(--purple)}
.ticon.r{background:var(--red-dim);color:var(--red)}
.ticon.a{background:var(--amber-dim);color:var(--amber)}
.tlabel{font-size:11px;font-weight:600;color:var(--text3);text-transform:uppercase;letter-spacing:.07em;margin-bottom:4px}
.tvalue{font-size:26px;font-weight:700;font-family:var(--mono);color:var(--text);letter-spacing:-.02em;line-height:1}
.tsub{font-size:11px;color:var(--text3);margin-top:5px;font-family:var(--mono)}

/* ── badge ── */
.badge{display:inline-flex;align-items:center;gap:5px;padding:4px 11px;border-radius:20px;font-size:11px;font-weight:700;letter-spacing:.05em;text-transform:uppercase}
.badge-auto{background:var(--green-dim);color:var(--green-t);border:1px solid rgba(34,197,94,.25)}
.badge-reject{background:var(--red-dim);color:var(--red-t);border:1px solid rgba(239,68,68,.25)}
.bdot{width:5px;height:5px;border-radius:50%;background:currentColor}

/* ── chip ── */
.chip{display:inline-flex;align-items:center;padding:2px 8px;border-radius:5px;font-size:10px;font-weight:600;font-family:var(--mono);letter-spacing:.04em;background:var(--bg4);border:1px solid var(--border2);color:var(--text3)}

/* ── file rows ── */
.ftable{width:100%}
.frow{display:flex;align-items:center;padding:9px 0;border-bottom:1px solid var(--border)}
.frow:first-child{padding-top:2px}
.frow:last-child{border-bottom:none;padding-bottom:2px}
.ficon{width:30px;height:30px;background:var(--bg4);border:1px solid var(--border);border-radius:7px;display:flex;align-items:center;justify-content:center;flex-shrink:0;margin-right:11px}
.ficon svg{width:13px;height:13px;stroke:var(--text3);fill:none;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}
.fmain{flex:1;min-width:0}
.fname{font-family:var(--mono);font-size:12px;color:var(--text);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;transition:color .15s}
.frow:hover .fname{color:var(--blue-t)}
.fmeta{font-size:11px;color:var(--text3);margin-top:2px}
.faction{display:flex;gap:6px;flex-shrink:0;margin-left:10px}

/* ── buttons ── */
.btn{padding:6px 13px;font-size:12px;font-weight:500;border:1px solid var(--border2);border-radius:7px;cursor:pointer;background:var(--bg3);color:var(--text2);font-family:var(--sans);transition:all .15s;text-decoration:none;display:inline-flex;align-items:center;gap:5px;white-space:nowrap}
.btn:hover{background:var(--border);color:var(--text);border-color:var(--border3)}
.btn svg{width:12px;height:12px;stroke:currentColor;fill:none;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}
.btn-g{background:var(--green-dim);color:var(--green-t);border-color:rgba(34,197,94,.25)}
.btn-g:hover{background:rgba(34,197,94,.1);border-color:rgba(34,197,94,.4);color:var(--green-t)}
.btn-b{background:var(--blue-dim);color:var(--blue-t);border-color:rgba(59,130,246,.25)}
.btn-b:hover{background:rgba(59,130,246,.1);border-color:rgba(59,130,246,.4);color:var(--blue-t)}
.btn-full{width:100%;justify-content:center;padding:9px;margin-top:12px;display:flex}

/* ── storage bar ── */
.sbar-bg{height:6px;background:var(--bg4);border-radius:4px;overflow:hidden;border:1px solid var(--border);margin:10px 0 6px}
.sbar-fill{height:100%;border-radius:4px;background:linear-gradient(90deg,var(--blue),#6366f1);transition:width .5s ease}
.sbar-labels{display:flex;justify-content:space-between;font-size:11px;color:var(--text3);font-family:var(--mono)}

/* ── mini chart ── */
.chart{display:flex;align-items:flex-end;gap:4px;height:52px;padding:0 1px;margin-top:12px}
.cbar{flex:1;min-width:6px;border-radius:3px 3px 0 0;background:var(--bg4);border:1px solid var(--border2);border-bottom:none;transition:all .2s;cursor:default;min-height:4px}
.cbar:hover{background:var(--border3);border-color:var(--border3)}
.cbar.curr{background:linear-gradient(to top,rgba(34,197,94,.18),rgba(34,197,94,.06));border-color:rgba(34,197,94,.4)}
.clabels{display:flex;gap:4px;padding:0 1px;margin-top:2px}
.clabel{flex:1;text-align:center;font-size:9px;font-family:var(--mono);color:var(--text3);overflow:hidden}

/* ── divider ── */
hr{border:none;border-top:1px solid var(--border);margin:14px 0}

/* ── file content ── */
pre{font-family:var(--mono);font-size:12px;background:var(--bg3);border:1px solid var(--border);border-radius:var(--r);padding:16px;overflow-x:auto;white-space:pre-wrap;word-break:break-all;color:var(--text);line-height:1.8;margin-top:2px}

/* ── page header ── */
.phd{margin-bottom:20px}
.phd h1{font-size:20px;font-weight:700;letter-spacing:-.02em}
.phd p{font-size:13px;color:var(--text2);margin-top:4px}

/* ── back link ── */
.back{display:inline-flex;align-items:center;gap:6px;font-size:13px;color:var(--text2);text-decoration:none;margin-bottom:18px;transition:color .15s}
.back:hover{color:var(--text)}
.back svg{width:14px;height:14px;stroke:currentColor;fill:none;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}

/* ── empty state ── */
.empty{text-align:center;padding:36px 16px;color:var(--text3)}
.empty svg{width:28px;height:28px;stroke:var(--border3);fill:none;stroke-width:1.5;stroke-linecap:round;stroke-linejoin:round;display:block;margin:0 auto 10px}
.empty p{font-size:13px}

/* ── error ── */
.err-card{text-align:center;padding:60px 20px}
.err-code{font-family:var(--mono);font-size:60px;font-weight:700;color:var(--border3);letter-spacing:-.04em}
.err-msg{font-size:15px;color:var(--text2);margin-top:10px;margin-bottom:20px}

/* ── footer ── */
.footer{text-align:center;padding:24px;font-size:11px;color:var(--text3);font-family:var(--mono);border-top:1px solid var(--border);margin-top:4px}
.footer span{color:var(--border3)}
    )css");
}

// ─── HTML head ───────────────────────────────────────────────────────────────
String htmlHead(const String& title = "Rejection Bin") {
    return R"rawliteral(<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>)rawliteral" + title + R"rawliteral( — Rejection Bin</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400;600&display=swap" rel="stylesheet">
<link rel="stylesheet" href="/style.css">
</head><body>)rawliteral";
}

// ─── Topbar + Nav ────────────────────────────────────────────────────────────
String htmlNav(const String& activePage) {
    bool isDash     = (activePage == "dashboard");
    bool isSessions = (activePage == "sessions");
    bool isFiles    = (activePage == "files");

    return R"rawliteral(
<div class="topbar">
  <div class="brand">
    <div class="logo">
      <svg viewBox="0 0 16 16"><rect x="2" y="6" width="12" height="9" rx="1"/><path d="M1 6l2-5h10l2 5"/><line x1="8" y1="6" x2="8" y2="15"/></svg>
    </div>
    <div>
      <div class="brand-name">Rejection Bin</div>
      <div class="brand-sub">192.168.1.21</div>
    </div>
  </div>
  <div class="topbar-right">
    <div class="pill-online">
      <div class="pulse"></div>
      <span class="pill-label">ONLINE</span>
    </div>
    <a href="/downloadall" class="btn btn-g">
      <svg viewBox="0 0 16 16"><path d="M8 2v8M5 7l3 3 3-3"/><path d="M3 11v2a1 1 0 001 1h8a1 1 0 001-1v-2"/></svg>
      Export
    </a>
  </div>
</div>
<div class="nav">
  <a href="/" class=")rawliteral" + String(isDash ? "active" : "") + R"rawliteral(">
    <svg class="ni" viewBox="0 0 16 16"><rect x="1" y="1" width="6" height="6" rx="1"/><rect x="9" y="1" width="6" height="6" rx="1"/><rect x="1" y="9" width="6" height="6" rx="1"/><rect x="9" y="9" width="6" height="6" rx="1"/></svg>
    Dashboard
  </a>
  <a href="/sessions" class=")rawliteral" + String(isSessions ? "active" : "") + R"rawliteral(">
    <svg class="ni" viewBox="0 0 16 16"><path d="M2 4h12M2 8h12M2 12h8"/></svg>
    Sessions
  </a>
  <a href="/files" class=")rawliteral" + String(isFiles ? "active" : "") + R"rawliteral(">
    <svg class="ni" viewBox="0 0 16 16"><path d="M9 1H3a1 1 0 00-1 1v12a1 1 0 001 1h10a1 1 0 001-1V6L9 1z"/><polyline points="9,1 9,6 14,6"/></svg>
    Files
  </a>
</div>)rawliteral";
}

String htmlFooter() {
    return "<div class='footer'>Rejection Bin <span>&middot;</span> ESP32 &amp; SPIFFS <span>&middot;</span> 192.168.1.21</div></body></html>";
}

// ─── Reusable icon snippets ───────────────────────────────────────────────────
static const char DL_ICON[]   = "<svg viewBox='0 0 16 16'><path d='M8 2v8M5 7l3 3 3-3'/><path d='M3 11v2a1 1 0 001 1h8a1 1 0 001-1v-2'/></svg>";
static const char FILE_ICON[] = "<svg viewBox='0 0 16 16'><path d='M9 1H3a1 1 0 00-1 1v12a1 1 0 001 1h10a1 1 0 001-1V6L9 1z'/><polyline points='9,1 9,6 14,6'/></svg>";
static const char LIST_ICON[] = "<svg viewBox='0 0 16 16'><path d='M2 4h12M2 8h12M2 12h8'/></svg>";

// Stream file bytes directly to current HTTP client (avoids temporary String churn)
void streamFileToClient(File &file)
{
    const size_t BUF = 128;
    uint8_t buf[BUF];
    WiFiClient client = server.client();

    while (file.available())
    {
        size_t n = file.read(buf, BUF);
        if (n > 0)
        {
            client.write(buf, n);
        }
    }
}

// ─── Dashboard ───────────────────────────────────────────────────────────────
void handleRoot() {
    size_t total = SPIFFS.totalBytes();
    size_t used  = SPIFFS.usedBytes();
    int    pct   = (total > 0) ? (int)((used * 100) / total) : 0;

    String modeClass = state.machine_mode ? "badge-reject" : "badge-auto";
    String modeLabel = state.machine_mode ? "REJECT" : "AUTO";
    String modeIcon  = state.machine_mode ? "r" : "g";

    String html = htmlHead("Dashboard");
    html.reserve(16384);
    html += htmlNav("dashboard");
    html += "<div class='content'>";

    // ── Metric tiles
    html += "<div class='g4'>";

    html += "<div class='tile'>";
    html += "<div class='ticon b'><svg viewBox='0 0 16 16'><circle cx='8' cy='8' r='6'/><polyline points='8,5 8,8 10,10'/></svg></div>";
    html += "<div class='tlabel'>Boot</div><div class='tvalue'>" + String(current_boot_number) + "</div>";
    html += "<div class='tsub'>session #</div></div>";

    html += "<div class='tile'>";
    html += "<div class='ticon p'><svg viewBox='0 0 16 16'><path d='M3 13V7a5 5 0 0110 0v6'/><line x1='1' y1='13' x2='15' y2='13'/></svg></div>";
    html += "<div class='tlabel'>This session</div><div class='tvalue'>" + String(current_session_count) + "</div>";
    html += "<div class='tsub'>rejections</div></div>";

    html += "<div class='tile'>";
    html += "<div class='ticon g'><svg viewBox='0 0 16 16'><polyline points='1,12 5,7 9,9 15,3'/></svg></div>";
    html += "<div class='tlabel'>Lifetime total</div><div class='tvalue'>" + String(total_lifetime_count) + "</div>";
    html += "<div class='tsub'>all boots</div></div>";

    html += "<div class='tile'>";
    html += "<div class='ticon " + modeIcon + "'><svg viewBox='0 0 16 16'><circle cx='8' cy='8' r='3'/><path d='M8 1v2M8 13v2M1 8h2M13 8h2M3.2 3.2l1.4 1.4M11.4 11.4l1.4 1.4M3.2 12.8l1.4-1.4M11.4 4.6l1.4-1.4'/></svg></div>";
    html += "<div class='tlabel'>Mode</div>";
    html += "<div style='margin-top:7px'><span class='badge " + modeClass + "'><span class='bdot'></span>" + modeLabel + "</span></div></div>";

    html += "</div>"; // g4

    // ── Two-column row
    html += "<div class='g2'>";

    // Core files card
    html += "<div class='card'><div class='card-hd'><div class='card-title'>Core files</div></div><div class='ftable'>";
    String coreFiles[] = {"/boot_number.txt", "/total_count.txt", "/state.txt"};
    for (int i = 0; i < 3; i++) {
        if (SPIFFS.exists(coreFiles[i])) {
            File f = SPIFFS.open(coreFiles[i]);
            size_t sz = f.size();
            f.close();
            html += "<div class='frow'><div class='ficon'>" + String(FILE_ICON) + "</div>";
            html += "<div class='fmain'><div class='fname'>" + coreFiles[i] + "</div><div class='fmeta'>" + String(sz) + " B</div></div>";
            html += "<div class='faction'>";
            html += "<a href='/view?file=" + coreFiles[i] + "' class='btn'>View</a>";
            html += "<a href='/download?file=" + coreFiles[i] + "' class='btn btn-b'>" + String(DL_ICON) + "</a>";
            html += "</div></div>";
        }
    }
    html += "</div></div>";

    // Storage + chart card
    html += "<div class='card'>";
    html += "<div class='card-hd'><div class='card-title'>Storage</div><div class='chip'>" + String(pct) + "%&nbsp;used</div></div>";
    html += "<div class='sbar-bg'><div class='sbar-fill' style='width:" + String(pct) + "%'></div></div>";
    html += "<div class='sbar-labels'><span>" + String(used / 1024.0 / 1024.0, 2) + " MB</span><span>" + String(total / 1024.0 / 1024.0, 2) + " MB total</span></div>";
    html += "<hr><div class='card-title' style='margin-bottom:2px'>Rejections per boot</div>";
    html += "<div class='chart'>";

    int maxCount = 1;
    int counts[8] = {0};
    int startBoot = (int)max((long unsigned int)1, current_boot_number - 7);
    int numBars   = (int)(current_boot_number - startBoot + 1);
    for (int i = startBoot; i <= (int)current_boot_number; i++) {
        String path = "/start_" + String(i) + ".txt";
        if (SPIFFS.exists(path)) {
            File f = SPIFFS.open(path);
            int cnt = 0;
            while (f.available()) { f.read(); cnt++; }
            f.close();
            counts[i - startBoot] = cnt;
            if (cnt > maxCount) maxCount = cnt;
        }
    }
    for (int i = 0; i < numBars; i++) {
        int h = max(8, (counts[i] * 100) / maxCount);
        bool isCurrent = ((startBoot + i) == (int)current_boot_number);
        html += "<div class='cbar" + String(isCurrent ? " curr" : "") + "' style='height:" + String(h) + "%' title='Boot " + String(startBoot + i) + ": " + String(counts[i]) + "'></div>";
    }
    html += "</div><div class='clabels'>";
    for (int i = 0; i < numBars; i++) {
        html += "<div class='clabel'>" + String(startBoot + i) + "</div>";
    }
    html += "</div></div>"; // card

    html += "</div>"; // g2

    // ── Recent sessions
    html += "<div class='g1'><div class='card'>";
    html += "<div class='card-hd'><div class='card-title'>Recent sessions</div></div>";
    html += "<div class='ftable'>";
    int count = 0;
    for (int i = (int)current_boot_number; i >= 1 && count < 5; i--) {
        String path = "/start_" + String(i) + ".txt";
        if (SPIFFS.exists(path)) {
            File f = SPIFFS.open(path);
            size_t sz = f.size();
            f.close();
            bool isCurr = ((uint32_t)i == current_boot_number);
            html += "<div class='frow'><div class='ficon'>" + String(LIST_ICON) + "</div>";
            html += "<div class='fmain'><div class='fname'>Boot " + String(i) + (isCurr ? "&nbsp;<span class='chip'>current</span>" : "") + "</div>";
            html += "<div class='fmeta'>" + path + " &nbsp;&middot;&nbsp; " + String(sz) + " B</div></div>";
            html += "<div class='faction'>";
            html += "<a href='/view?file=" + path + "' class='btn'>View</a>";
            html += "<a href='/download?file=" + path + "' class='btn btn-b'>" + String(DL_ICON) + "</a>";
            html += "</div></div>";
            count++;
        }
    }
    if (count == 0) {
        html += "<div class='empty'><svg viewBox='0 0 16 16'><path d='M2 4h12M2 8h12M2 12h8'/></svg><p>No session files yet</p></div>";
    }
    html += "</div>";
    html += "<a href='/sessions' class='btn btn-full'>View all sessions &nbsp;<svg viewBox='0 0 16 16'><polyline points='6,3 11,8 6,13'/></svg></a>";
    html += "</div></div>"; // card + g1

    html += "</div>"; // content
    html += htmlFooter();
    server.send(200, "text/html", html);
}

// ─── Sessions page ───────────────────────────────────────────────────────────
void handleSessions() {
    String html = htmlHead("Sessions");
    html.reserve(8192);
    html += htmlNav("sessions");
    html += "<div class='content'>";
    html += "<div class='phd'><h1>Sessions</h1><p>All boot session files stored in SPIFFS</p></div>";
    html += "<div class='card'>";
    html += "<div class='card-hd'><div class='card-title'>Session files</div><div class='chip'>Boot&nbsp;" + String(current_boot_number) + "</div></div>";
    html += "<div class='ftable'>";

    bool any = false;
    for (int i = (int)current_boot_number; i >= 1; i--) {
        String path = "/start_" + String(i) + ".txt";
        if (SPIFFS.exists(path)) {
            File f = SPIFFS.open(path);
            size_t sz = f.size();
            f.close();
            bool isCurr = ((uint32_t)i == current_boot_number);
            html += "<div class='frow'><div class='ficon'>" + String(LIST_ICON) + "</div>";
            html += "<div class='fmain'><div class='fname'>Boot " + String(i) + (isCurr ? "&nbsp;<span class='chip'>current</span>" : "") + "</div>";
            html += "<div class='fmeta'>" + path + " &nbsp;&middot;&nbsp; " + String(sz) + " B</div></div>";
            html += "<div class='faction'>";
            html += "<a href='/view?file=" + path + "' class='btn'>View</a>";
            html += "<a href='/download?file=" + path + "' class='btn btn-b'>" + String(DL_ICON) + "</a>";
            html += "</div></div>";
            any = true;
        }
    }
    if (!any) {
        html += "<div class='empty'><svg viewBox='0 0 16 16'><path d='M2 4h12M2 8h12M2 12h8'/></svg><p>No session files found</p></div>";
    }
    html += "</div></div>";
    html += "</div>";
    html += htmlFooter();
    server.send(200, "text/html", html);
}

// ─── Files page ──────────────────────────────────────────────────────────────
void handleFiles()
{
    String html = htmlHead("Files");
    html.reserve(8192);
    html += htmlNav("files");
    html += "<div class='content'>";
    html += "<div class='phd'><h1>Files</h1><p>All files stored in the SPIFFS filesystem</p></div>";
    html += "<div class='card'>";
    html += "<div class='card-hd'><div class='card-title'>SPIFFS contents</div></div>";
    html += "<div class='ftable'>";

    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    bool any = false;
    while (file) {
        String name = "/" + String(file.name());
        html += "<div class='frow'><div class='ficon'>" + String(FILE_ICON) + "</div>";
        html += "<div class='fmain'><div class='fname'>" + name + "</div><div class='fmeta'>" + String(file.size()) + " B</div></div>";
        html += "<div class='faction'>";
        html += "<a href='/view?file=" + name + "' class='btn'>View</a>";
        html += "<a href='/download?file=" + name + "' class='btn btn-b'>" + String(DL_ICON) + "</a>";
        html += "</div></div>";
        file = root.openNextFile();
        any = true;
    }
    if (!any) {
        html += "<div class='empty'><svg viewBox='0 0 16 16'><path d='M9 1H3a1 1 0 00-1 1v12a1 1 0 001 1h10a1 1 0 001-1V6L9 1z'/><polyline points='9,1 9,6 14,6'/></svg><p>No files found</p></div>";
    }
    html += "</div></div>";
    html += "</div>";
    html += htmlFooter();
    server.send(200, "text/html", html);
}

// ─── View file ───────────────────────────────────────────────────────────────
void handleView() {
    String filepath = server.arg("file");

    if (!isValidPath(filepath)) {
        server.send(400, "text/html",
            htmlHead("Error") + htmlNav("") +
            "<div class='content'><div class='card err-card'><div class='err-code'>400</div><div class='err-msg'>Invalid file path</div><a href='/' class='btn btn-g'>Go home</a></div></div>" +
            htmlFooter());
        return;
    }
    if (!SPIFFS.exists(filepath)) {
        server.send(404, "text/html",
            htmlHead("Not Found") + htmlNav("") +
            "<div class='content'><div class='card err-card'><div class='err-code'>404</div><div class='err-msg'>File not found</div><a href='/' class='btn btn-g'>Go home</a></div></div>" +
            htmlFooter());
        return;
    }

    File file = SPIFFS.open(filepath);
    if (!file) {
        server.send(500, "text/html",
            htmlHead("Error") + htmlNav("") +
            "<div class='content'><div class='card err-card'><div class='err-code'>500</div><div class='err-msg'>Cannot open file</div></div></div>" +
            htmlFooter());
        return;
    }

    String content = "";
    content.reserve(file.size() + 1);
    while (file.available()) content += (char)file.read();
    size_t sz = content.length();
    file.close();

    String html = htmlHead(filepath);
    html.reserve(6144 + content.length());
    html += htmlNav("");
    html += "<div class='content'>";
    html += "<a href='javascript:history.back()' class='back'><svg viewBox='0 0 16 16'><polyline points='10,3 5,8 10,13'/></svg> Back</a>";
    html += "<div class='phd'><h1>" + filepath + "</h1><p>" + String(sz) + " bytes</p></div>";
    html += "<div class='card'>";
    html += "<div class='card-hd'><div class='card-title'>Contents</div><a href='/download?file=" + filepath + "' class='btn btn-b'>" + String(DL_ICON) + " Download</a></div>";
    html += "<pre>" + content + "</pre>";
    html += "</div>";
    html += "</div>";
    html += htmlFooter();
    server.send(200, "text/html", html);
}

// ─── Download single file ────────────────────────────────────────────────────
void handleDownload() {
    String filepath = server.arg("file");

    if (!isValidPath(filepath))       { server.send(400, "text/plain", "Invalid path");   return; }
    if (!SPIFFS.exists(filepath))     { server.send(404, "text/plain", "File not found"); return; }

    File file = SPIFFS.open(filepath);
    if (!file)                        { server.send(500, "text/plain", "Cannot open");    return; }

    String filename = filepath;
    filename.replace("/", "");
    server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
    server.streamFile(file, "application/octet-stream");
    file.close();
}

// ─── Download all (streamed) ─────────────────────────────────────────────────
void handleDownloadAll() {
    server.sendHeader("Content-Disposition", "attachment; filename=rejection_data.txt");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/plain", "");

    server.sendContent("=== REJECTION BIN SYSTEM DATA ===\r\n");
    server.sendContent("Boot: " + String(current_boot_number) + " | Total: " + String(total_lifetime_count) + "\r\n\r\n");

    server.sendContent("=== CORE FILES ===\r\n");
    const char* coreFiles[] = {"/boot_number.txt", "/total_count.txt", "/state.txt"};
    for (int i = 0; i < 3; i++) {
        if (SPIFFS.exists(coreFiles[i])) {
            File f = SPIFFS.open(coreFiles[i]);
            server.sendContent(coreFiles[i] + ": ");
            streamFileToClient(f);
            f.close();
            server.sendContent("\r\n");
        }
    }

    server.sendContent("\r\n=== SESSION FILES ===\r\n");
    for (int i = 1; i <= (int)current_boot_number; i++) {
        String path = "/start_" + String(i) + ".txt";
        if (SPIFFS.exists(path)) {
            File f = SPIFFS.open(path);
            server.sendContent("Boot " + String(i) + ": ");
            streamFileToClient(f);
            f.close();
            server.sendContent("\r\n");
        }
    }
    server.sendContent("");
}

// ─── 404 handler ─────────────────────────────────────────────────────────────
void handleNotFound() {
    server.send(404, "text/html",
        htmlHead("Not Found") + htmlNav("") +
        "<div class='content'><div class='card err-card'><div class='err-code'>404</div><div class='err-msg'>Page not found</div><a href='/' class='btn btn-g'>Go home</a></div></div>" +
        htmlFooter());
}

// ─── Init ─────────────────────────────────────────────────────────────────────
void initWebServer() {
    Serial.println("=== WEB SERVER INIT ===");

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    WiFi.softAP(AP_SSID, AP_PASSWORD);

    Serial.printf("SSID:     %s\n", AP_SSID);
    Serial.printf("Password: %s\n", AP_PASSWORD);
    Serial.printf("IP:       %s\n", WiFi.softAPIP().toString().c_str());
    Serial.println("Open: http://" + WiFi.softAPIP().toString());
    Serial.println("=======================");

    server.on("/",            handleRoot);
    server.on("/sessions",    handleSessions);
    server.on("/files",       handleFiles);
    server.on("/view",        handleView);
    server.on("/download",    handleDownload);
    server.on("/downloadall", handleDownloadAll);
    server.on("/style.css",   handleCSS);
    server.onNotFound(handleNotFound);

    server.begin();
}

#endif