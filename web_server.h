// web_server.h - Speed-optimized web portal for SPIFFS data
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFi.h>
#include <WebServer.h>
#include "config.h"
#include "FS.h"
#include "SPIFFS.h"

#define AP_SSID "RejectionBin_AP"
#define AP_PASSWORD "rejectionbin"
#define AP_IP IPAddress(192, 168, 1, 21)
#define AP_GATEWAY IPAddress(192, 168, 1, 21)
#define AP_SUBNET IPAddress(255, 255, 255, 0)

WebServer server(80);

// Minimal HTML header with ultra-light CSS
String htmlHeader() {
    return R"rawliteral(<!DOCTYPE html><html><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Rejection Bin</title>
<style>
body{font-family:Arial;margin:15px;background:#f5f5f5}
.card{background:#fff;padding:12px;margin:8px 0;border-radius:3px;box-shadow:0 1px 3px rgba(0,0,0,0.1)}
h1{font-size:18px;margin:0 0 5px 0;color:#333}
h2{font-size:15px;margin:0 0 8px 0;color:#333;border-bottom:2px solid #4CAF50;padding-bottom:4px}
.info{display:flex;justify-content:space-between;padding:6px 0;border-bottom:1px solid #eee}
.info:last-child{border:none}
.label{font-weight:bold;color:#666}
.value{color:#333}
.file{padding:8px;margin:4px 0;background:#f9f9f9;border-left:3px solid #4CAF50}
.file-name{font-family:monospace;display:block;margin-bottom:4px}
.btn{padding:6px 12px;margin:2px;border:none;border-radius:3px;cursor:pointer;font-size:13px;text-decoration:none;display:inline-block}
.btn-v{background:#4CAF50;color:#fff}
.btn-d{background:#2196F3;color:#fff}
.btn-b{background:#ff9800;color:#fff;width:100%;padding:10px;margin-top:8px}
.menu{background:#fff;padding:8px;margin:8px 0;border-radius:3px}
.menu a{background:#4CAF50;color:#fff;padding:8px 12px;margin:2px;border-radius:3px;text-decoration:none;font-size:13px;display:inline-block}
@media(max-width:768px){
.file{display:block}
.btn{width:100%;margin:3px 0}
.menu a{width:100%;text-align:center;display:block;margin:4px 0}
}
</style>
</head><body>)rawliteral";
}

String htmlFooter() {
    return "</body></html>";
}

// Dashboard - optimized for speed
void handleRoot() {
    String html = htmlHeader();
    
    html += "<div class='card'><h1>🏭 REJECTION BIN SYSTEM</h1></div>";
    
    html += "<div class='menu'>";
    html += "<a href='/'>Dashboard</a>";
    html += "<a href='/sessions'>All Sessions</a>";
    html += "<a href='/downloadall'>Download All</a>";
    html += "</div>";
    
    html += "<div class='card'><h2>📊 Status</h2>";
    html += "<div class='info'><span class='label'>Boot:</span><span class='value'>" + String(current_boot_number) + "</span></div>";
    html += "<div class='info'><span class='label'>Session:</span><span class='value'>" + String(current_session_count) + "</span></div>";
    html += "<div class='info'><span class='label'>Total:</span><span class='value'>" + String(total_lifetime_count) + "</span></div>";
    html += "<div class='info'><span class='label'>Mode:</span><span class='value'>" + String(state.machine_mode ? "REJECT" : "AUTO") + "</span></div>";
    html += "</div>";
    
    html += "<div class='card'><h2>📁 Core Files</h2>";
    String coreFiles[] = {"/boot_number.txt", "/total_count.txt", "/state.txt"};
    for (int i = 0; i < 3; i++) 
    {
        if (SPIFFS.exists(coreFiles[i])) 
        {
            File f = SPIFFS.open(coreFiles[i]);
            html += "<div class='file'><span class='file-name'>" + coreFiles[i] + " (" + String(f.size()) + "B)</span>";
            html += "<a href='/view?file=" + coreFiles[i] + "' class='btn btn-v'>View</a>";
            html += "<a href='/download?file=" + coreFiles[i] + "' class='btn btn-d'>Download</a></div>";
            f.close();
        }
    }
    html += "</div>";
    
    html += "<div class='card'><h2>📁 Recent Sessions</h2>";
    int count = 0;
    int startFrom = (current_boot_number > 3) ? current_boot_number - 2 : 1;
    for (int i = current_boot_number; i >= startFrom && count < 3; i--) {
        String path = "/start_" + String(i) + ".txt";
        if (SPIFFS.exists(path)) 
        {
            File f = SPIFFS.open(path);
            html += "<div class='file'><span class='file-name'>" + path + " (" + String(f.size()) + "B)</span>";
            html += "<a href='/view?file=" + path + "' class='btn btn-v'>View</a>";
            html += "<a href='/download?file=" + path + "' class='btn btn-d'>Download</a></div>";
            f.close();
            count++;
        }
    }
    html += "<a href='/sessions' class='btn btn-b'>View All Sessions</a></div>";
    
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    html += "<div class='card'><h2>💾 Storage</h2>";
    html += "<div class='info'><span class='label'>Total:</span><span class='value'>" + String(total/1024.0/1024.0, 2) + " MB</span></div>";
    html += "<div class='info'><span class='label'>Used:</span><span class='value'>" + String(used/1024.0/1024.0, 2) + " MB</span></div>";
    html += "<div class='info'><span class='label'>Free:</span><span class='value'>" + String((total-used)/1024.0/1024.0, 2) + " MB</span></div>";
    html += "</div>";
    
    html += htmlFooter();
    server.send(200, "text/html", html);
}

// All sessions page
void handleSessions() {
    String html = htmlHeader();
    
    html += "<div class='card'><h1>📁 ALL SESSIONS</h1></div>";
    html += "<div class='menu'><a href='/'>← Dashboard</a></div>";
    html += "<div class='card'><h2>Session Files</h2>";
    
    for (int i = 1; i <= current_boot_number; i++) {
        String path = "/start_" + String(i) + ".txt";
        if (SPIFFS.exists(path)) {
            File f = SPIFFS.open(path);
            html += "<div class='file'><span class='file-name'>Boot " + String(i) + " - " + path + " (" + String(f.size()) + "B)</span>";
            html += "<a href='/view?file=" + path + "' class='btn btn-v'>View</a>";
            html += "<a href='/download?file=" + path + "' class='btn btn-d'>Download</a></div>";
            f.close();
        }
    }
    
    html += "</div>";
    html += htmlFooter();
    server.send(200, "text/html", html);
}

// View file
void handleView() {
    String filepath = server.arg("file");
    
    if (!SPIFFS.exists(filepath)) {
        server.send(404, "text/html", htmlHeader() + "<div class='card'><h2>Error</h2><p>File not found</p></div>" + htmlFooter());
        return;
    }
    
    File file = SPIFFS.open(filepath);
    if (!file) {
        server.send(500, "text/html", htmlHeader() + "<div class='card'><h2>Error</h2><p>Cannot open file</p></div>" + htmlFooter());
        return;
    }
    
    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }
    file.close();
    
    String html = htmlHeader();
    html += "<div class='card'><h1>📄 " + filepath + "</h1></div>";
    html += "<div class='menu'><a href='/'>← Dashboard</a><a href='/download?file=" + filepath + "'>Download</a></div>";
    html += "<div class='card'><h2>Content</h2>";
    html += "<div class='info'><span class='label'>Size:</span><span class='value'>" + String(content.length()) + " bytes</span></div>";
    html += "<pre style='background:#f5f5f5;padding:10px;border-radius:3px;overflow-x:auto'>" + content + "</pre>";
    html += "</div>";
    html += htmlFooter();
    server.send(200, "text/html", html);
}

// Download file
void handleDownload() {
    String filepath = server.arg("file");
    
    if (!SPIFFS.exists(filepath)) {
        server.send(404, "text/plain", "File not found");
        return;
    }
    
    File file = SPIFFS.open(filepath);
    if (!file) {
        server.send(500, "text/plain", "Cannot open file");
        return;
    }
    
    String filename = filepath;
    filename.replace("/", "");
    
    server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
    server.streamFile(file, "application/octet-stream");
    file.close();
}

// Download all files
void handleDownloadAll() {
    String data = "=== REJECTION BIN SYSTEM DATA ===\n";
    data += "Boot: " + String(current_boot_number) + " | Total: " + String(total_lifetime_count) + "\n\n";
    
    data += "=== CORE FILES ===\n";
    String coreFiles[] = {"/boot_number.txt", "/total_count.txt", "/state.txt"};
    for (int i = 0; i < 3; i++) {
        if (SPIFFS.exists(coreFiles[i])) {
            File f = SPIFFS.open(coreFiles[i]);
            data += coreFiles[i] + ": ";
            while (f.available()) data += (char)f.read();
            data += "\n";
            f.close();
        }
    }
    
    data += "\n=== SESSION FILES ===\n";
    for (int i = 1; i <= current_boot_number; i++) 
    {
        String path = "/start_" + String(i) + ".txt";
        if (SPIFFS.exists(path)) 
        {
            File f = SPIFFS.open(path);
            data += "Boot " + String(i) + ": ";
            while (f.available()) data += (char)f.read();
            data += "\n";
            f.close();
        }
    }
    
    server.sendHeader("Content-Disposition", "attachment; filename=rejection_data.txt");
    server.send(200, "text/plain", data);
}

// Initialize
void initWebServer() 
{
    Serial.println("=== WEB SERVER INIT ===");
    
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    Serial.printf("SSID: %s\n", AP_SSID);
    Serial.printf("Password: %s\n", AP_PASSWORD);
    Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.println("Open: http://" + WiFi.softAPIP().toString());
    Serial.println("=======================");
    
    server.on("/", handleRoot);
    server.on("/sessions", handleSessions);
    server.on("/view", handleView);
    server.on("/download", handleDownload);
    server.on("/downloadall", handleDownloadAll);
    
    server.begin();
}

#endif