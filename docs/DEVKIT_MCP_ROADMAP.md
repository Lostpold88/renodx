# RenoDX DevKit & MCP Roadmap – Analyse-Tool für SDR→HDR-Upgrades & HDR-Fixes

Dieses Dokument sammelt konkrete, **MCP-first** gedachte Erweiterungen für `renodx-devkit.addon64` (`src/addons/devkit/`), damit die komplette Analyse-, Debugging-, Prototyping- und Verifikationsschleife für neue HDR-Mods (SDR→HDR und Native-HDR-Fixes) möglichst schnell, reproduzierbar und ohne Performance-Verlust im Spiel funktioniert.

---

## 1. Performance & Capture-Steuerung (auf Basis des Idle-Modus)

### 1.1 MCP-Steuerung für `Idle` / `Active` + One-Shot-Snapshot
- **Neue/erweiterte Tools:**
  - `devkit_set_tracking_active({ active: bool })`
  - `devkit_status` liefert zusätzlich `trackingActive`, `startActive`, aktive Hook-Subsysteme
  - `devkit_queue_snapshot({ returnToIdle: true })`
- **Nutzen:** Das Spiel läuft dauerhaft mit vollen FPS im `Idle`-Modus. Fordert MCP einen Snapshot an, werden die Hot-Path-Hooks nur für den 1 Capture-Frame aktiviert und direkt nach Abschluss automatisch wieder deregistriert.

### 1.2 Letzten Rest-Overhead im Idle-Modus entfernen
- Die 3 verbleibenden Per-Render-Pass-Hooks in `src/utils/swapchain.hpp` (`bind_render_targets_and_depth_stencil`, `begin_render_pass`, `end_render_pass`) im Idle-Modus ebenfalls über `shared.RegisterEvent(..., active)` abschalten.

---

## 2. Bereits in der DevKit-UI vorhanden – aber noch nicht per MCP exponiert (Quick Wins)

Diese Funktionen existieren bereits im DevKit-Code (`src/addons/devkit/addon.cpp`), sind aber bislang nur über die ImGui-Oberfläche erreichbar:

### 2.1 `devkit_list_resources` (komplette Ressourcen-Liste + Filter)
- **Heute:** Man muss Resource-Handles umständlich über einzelne `devkit_get_draw`-Calls zusammensuchen.
- **Neu:** Alle getrackten Ressourcen auflisten mit Filtern (`type`: `texture_2d` / `texture_3d` / `buffer`, `format`, `minWidth`, `minHeight`, `seenRtv`, `seenUav`, `seenSrv`, `isBackbuffer`).

### 2.2 `devkit_set_shader_bypass` & `devkit_list_bypassed_shaders`
- **Heute:** `ShaderDetails::bypass_draw` existiert bereits in `addon.cpp`, aber nur per UI-Checkbox.
- **Neu:** Einzelne Shader-Hashes per MCP stummschalten, um sofort zu testen, ob ein Pass UI/HUD, Bloom, Vignette, Film Grain, Chromatic Aberration oder Tonemapping ist – kombinierbar mit einem direkten Backbuffer-Readback.

### 2.3 `devkit_get_swapchain_info` (Output- & HDR-Signaling-Diagnose)
- **Heute:** Infos liegen teilweise in `RenderInfoPane`, aber nicht strukturiert im MCP.
- **Neu:** Ein einzelner Call liefert:
  - Grafik-API (DX9 / DX11 / DX12 / Vulkan / OpenGL)
  - SwapChain-Format (`RGBA8`, `RGB10A2`, `RGBA16F`), Auflösung, Buffer-Count, Backbuffer-Handles
  - DXGI/Vulkan-ColorSpace (`BT709_G22`, `BT2020_PQ`, `BT709_G10` scRGB)
  - OS-HDR-Status, gesendete HDR-Metadaten (MaxCLL / MaxFALL / Mastering Display), Flip-Model-Status
  - Proxy-SwapChain-Status (`utils::device_proxy`)

### 2.4 `devkit_queue_trace` & `devkit_get_trace_log`
- **Heute:** `utils::trace::trace_scheduled_device` kann nur über den Menü-Button `Trace` ausgelöst werden und landet unstrukturiert in `ReShade.log`.
- **Neu:** 1-Frame-API-Trace per MCP auslösen (inkl. Auto-Idle danach) und gefiltert/strukturiert als JSON-Einträge abrufen.

### 2.5 `devkit_get_device_proxy_state` & `devkit_set_device_proxy_mode`
- **Heute:** SwapChain-Device-Proxy (SDR / HDR10 / scRGB + Custom Proxy Shaders) ist bereits in der UI verdrahtet.
- **Neu:** Per MCP umschalten und Status auslesen, um ältere/SDR-Spiele (z. B. DX9/DX10/DX11 ohne natives HDR) direkt auf HDR10/scRGB zu testen.

### 2.6 Blend-State & Fullscreen-Flags in `devkit_get_draw` / `devkit_list_draws`
- Blend-Enable, Src/Dst-Blend-Faktoren, Viewport-Größe relativ zum Backbuffer und `isFullscreenPass`-Flag direkt mitliefern.
- **Nutzen:** Trennt UI/HUD, additive Bloom-/Partikel-Passes und echte Fullscreen-Post-Processing-Passes auf einen Blick.

---

## 3. Herzstück für SDR→HDR-Upgrades: Automatisierte Pipeline- & HDR-Analyse

### 3.1 `devkit_trace_output_chain` (Automatischer Backbuffer-Rückwärts-Tracer)
- Verfolgt vom SwapChain-Backbuffer automatisch rückwärts alle `RTV` / `UAV` / `CopyResource` / `CopyTextureRegion` → `SRV`-Verknüpfungen über den gesamten Snapshot.
- Liefert einen kompakten Post-Processing-DAG statt hunderter Einzel-Draws:
  - Reihenfolge der relevanten Passes (inkl. Shader-Hash, Stage, Format, Auflösung)
  - **UI/HUD-Grenze:** Exakter Draw-Index, ab dem UI-Elemente auf den Backbuffer gezeichnet werden
  - **Format-Flaschenhälse:** Erste `RGBA8_UNORM`-, `RGB10A2`- oder `R11G11B10F`-Targets (kein negativer Wertebereich → kaputtes Wide Gamut)
  - **HDR-Drop-Off-Pass:** Letzter Pass mit Werten `> 1.0` oder `< 0.0` vs. erster geclippter Pass

### 3.2 `devkit_scan_pass_resources` (Batch-HDR-, Gamut- & NaN-Scanner)
- **Heute:** Jeder Pass erfordert einen eigenen `devkit_analyze_resource`-Call.
- **Neu:** Eine Liste von Draw-Indices oder Resource-Handles in **einem** Frame-Readback analysieren.
- Pro Target direkt zurückgeben:
  - `min`, `max`, `mean`, `p99`, `p99.9` (pro Kanal & Luminanz)
  - `% > 1.0` (echte HDR-Highlights), `% < 0.0` (Wide-Gamut / negative Kanäle), `% NaN / Inf`
  - Gamut-Verteilung (`% BT.709`, `% DCI-P3`, `% BT.2020`, `% außerhalb BT.2020`)
- **Nutzen:** Findet in 2 Sekunden NaN-Quellen, unbemerkte `saturate()`-Clips und `R11G11B10F`-Clamping.

### 3.3 `devkit_classify_postprocess` (Heuristische Pass-Erkennung)
- Automatische Vor-Klassifizierung verdächtiger Passes im Output-Chain-Graphen:
  - `texture_3d`-SRV oder 2D-Strip (`256x16`, `1024x32`) → **LUT / Color Grading**
  - Mip-/Halb-Auflösungs-Kette mit additivem Blend → **Bloom / Downsample / Upsample**
  - PQ-/ST.2084-Konstanten (`0.15930175`, `78.84375`, `10000`) → **Native HDR10-Ausgabe**
  - 5-Tap-Nachbarschaft + `rcp`/`min`/`max` → **CAS / Sharpening**
  - Alpha-geblendete Quads auf Backbuffer nach dem letzten Fullscreen-Pass → **UI / HUD**

---

## 4. 3D-Texturen & LUT-Analyse per MCP

### 4.1 3D-Readback in `devkit_analyze_resource` + spezialisiertes `devkit_analyze_lut`
- **Heute:** `src/addons/devkit/mcp/resource_analysis.hpp` unterstützt nur `texture_2d`.
- **Neu:** Staging-Readback für `texture_3d` (z. B. 16³, 32³, 33³, 64³ in `RGBA8`, `RGB10A2`, `RGBA16F`, `RGBA32F`) und 2D-Strip-LUTs (`N² × N`).
- **Automatische LUT-Metriken per MCP:**
  - Wertebereich: Clippt die LUT auf `[0, 1]` (SDR), liefert sie HDR (`> 1.0`) oder negative WCG-Werte (`< 0.0`)?
  - **Neutralachse (`R = G = B`):** Komplette Graustufen-Kurve → Schwarzwert-Lift (`(0,0,0)`), Weißpunkt (`(1,1,1)`), Midgray-Shift, Kontrast-/S-Kurve, Erkennung ob Tonemapping bereits in die LUT eingebacken ist
  - **Identitäts-Abweichung:** Max/Mean-Delta zur Identity-LUT (erkennt No-Op-/Neutral-LUTs sofort)
  - **Primär-/Sekundärfarben-Analyse:** Sättigungs- und Hue-Verschiebung an R, G, B, C, M, Y
  - **Input-Space-Heuristik:** Schätzung, ob die LUT Linear, sRGB/Gamma 2.2, Log (UE/ACEScct) oder PQ erwartet
  - **Export:** Direkt als Standard-`.cube`-Datei, 2D-Unwrapped-EXR/PNG oder JSON-Samples für Kurven-Plots

---

## 5. Deep Reverse-Engineering: Konstanten, Shader & Snapshot-Diffs

### 5.1 `devkit_get_draw_constants` & `devkit_diff_draw_constants`
- **Problem:** In dekompilierten Shadern sieht man nur `cb0[14].xyzw` oder `push.data[3]`, aber nicht die Live-Werte.
- **Neu:**
  - Readback aller gebundenen CBVs / UBOs / Push-Constants eines Draws als `float`, `float4`, `uint`, `int`
  - **Snapshot-Diff:** Zwei Snapshots vergleichen (z. B. In-Game-HDR-Peak-Slider von 500 auf 1000 Nits gestellt oder Exposure geändert) → MCP meldet exakt CBV-Slot, Byte-Offset, alten Wert und neuen Wert.

### 5.2 `devkit_compare_snapshots` (Szenen- & Modus-Diff)
- Vergleicht zwei Snapshots (z. B. **In-Game HDR ON vs. OFF**, **CAS ON vs. OFF**, **Menü vs. Gameplay**, **Cutscene vs. In-Game**):
  - Welche Shader-Hashes kamen hinzu / fielen weg?
  - Welche RenderTarget-Formate haben sich geändert?
  - Wo weicht die Output-Chain ab?
- **Nutzen:** Findet sofort alternative Codepfade (wie z. B. CAS ON vs. OFF oder SDR- vs. HDR-Output-Shader in *Detroit: Become Human*).

### 5.3 `devkit_get_shader` mit Snapshot-Kontext & direkter Dekompilierung
- Optionale Rückgabe von Disassembly/Decompilation direkt im MCP-Response (mit Paging/Zeilenbereich) plus verknüpfter Binding-Tabelle (`t0` → `RGBA16F 2560x1440`, `t1` → `texture_3d 32x32x32 RGBA16F`, `u0` → …) direkt aus dem aktuellen Snapshot.

---

## 6. Live-Prototyping & Automatisierte Verifikation

### 6.1 `devkit_get_live_shader_status` & A/B-Metrik-Vergleich
- Strukturierte Rückmeldung nach `devkit_load_live_shaders`:
  - Compiler-Fehler und Warnungen pro Datei (Zeile, Spalte, Nachricht)
  - Tatsächlicher `drawCount` jedes ersetzten Shaders im letzten Frame (sofortige Bestätigung, ob der Patch überhaupt greift)
  - Optionales **`devkit_compare_resource_ab`**: Einmal mit Live-Shader aktiv, einmal ohne Live-Shader den Ziel-Pass/Backbuffer auslesen → direkte Tabelle mit Delta für Min/Max-Nits, Midgray, NaNs und Gamut-Abdeckung.

### 6.2 `devkit_check_upgrade_candidates` (Resource-Upgrade-Assistent)
- Analysiert alle `RGBA8`/`B8G8R8A8`/`R11G11B10F`/`RGB10A2`-Targets in der Post-Processing-Kette:
  - Prüft Usage-Flags (RTV, UAV, SRV, Copy, MSAA, Mips, Array, Aspect Ratio)
  - Meldet potentielle Risiken (z. B. `copy_resource`-Format-Mismatch, fehlendes `use_resource_cloning`)
  - Generiert direkt passende `renodx::mods::swapchain::ResourceUpgradeInfo`-Vorschläge für `addon.cpp`.

---

## Empfohlene Umsetzungsreihenfolge (Ausbaustufen)

| Stufe | Status | Fokus | Tools / Features | Aufwand | Hebel |
| :--- | :---: | :--- | :--- | :---: | :---: |
| **Stufe 1** | ✅ Umgesetzt | Quick Wins & Idle-Polish | `devkit_set_tracking_active`, `devkit_queue_snapshot(returnToIdle)`, `devkit_get_swapchain_info`, `devkit_list_resources`, `devkit_set_shader_bypass`, `utils::swapchain` Idle-Gating | Klein | Sehr hoch |
| **Stufe 2** | ✅ Umgesetzt | SDR→HDR-Kernanalyse | `devkit_trace_output_chain`, `devkit_scan_pass_resources` (Batch HDR/NaN/Gamut + p99/p99.9), Blend-/Backbuffer-/Fullscreen-Infos in `devkit_get_draw` / `devkit_list_draws` | Mittel | Extrem hoch |
| **Stufe 3** | ✅ Umgesetzt | 3D-Texturen, LUTs & Pass-Klassifizierung | 3D-Readback in `devkit_analyze_resource`, `devkit_analyze_lut` (Neutralachse, Clipping, Input-Space, Corner-Hue/Sat, `.cube`/`.json`/Preview-Export), `devkit_classify_postprocess` | Mittel | Sehr hoch |
| **Stufe 4** | ✅ Umgesetzt | Deep RE & Live-Verifikation | `devkit_get_draw_constants` + `devkit_diff_draw_constants`, `devkit_compare_snapshots`, `devkit_get_live_shader_status`, `devkit_compare_resource_ab`, `devkit_queue_trace` + `devkit_get_trace_log` | Mittel–Groß | Hoch |
| **Stufe 5** | ✅ Umgesetzt | Prototyping-Komfort | `devkit_check_upgrade_candidates` (Risiko-Analyse + Blast-Radius + `ResourceUpgradeInfo`-C++-Snippets), `devkit_get_device_proxy_state` + `devkit_set_device_proxy_mode` (SDR/HDR10/scRGB + Custom-FXC-Shaders auf Present-Thread), `devkit_get_shader` (Zeilen-Paging & `includeSnapshotContext`-Binding-Tabelle), `devkit_list_shaders(bypassedOnly)` (`devkit_classify_postprocess` bereits in Stufe 3 umgesetzt) | Mittel | Mittel–Hoch |

### Feinschliffe aus dem ersten Live-Test (Detroit: Become Human / Vulkan) – ✅ Umgesetzt
- **`src/utils/mcp/protocol.hpp`:** `structuredContent` wird zusätzlich als JSON im `content[0].text`-Block mitgeliefert, damit auch MCP-Clients, die nur `content[].text` anzeigen (z. B. Antigravity), alle strukturierten Ergebnisse direkt nativ sehen.
- **`devkit_trace_output_chain`:** Backbuffer mit `r10g10b10a2_unorm` + `hdr10_st2084`/`hdr10_hlg` werden nicht mehr fälschlich als Format-Flaschenhals markiert.
- **Statische Vulkan/DX12-Descriptor-Tables im Idle-Mode:** Neue Option `KeepDescriptorTrackingInIdle` (Standard `true`, in Settings → `Other` abschaltbar), damit SRV-Links in `devkit_trace_output_chain` auch nach einem Idle-Start vollständig sind.

### Feinschliffe aus den Live-Tests zu Stufe 5 (Detroit / Vulkan & WTTG3 / DX11) – ✅ Umgesetzt
- **DXGI-ABBA-Deadlock-Fix (`src/addons/devkit/addon.cpp`):** `color_space`, `current_backbuffer_index` und `backbuffer_handles` werden jetzt ausschließlich auf dem Render-/Present-Thread (`OnInitSwapchain` + `OnPresent`, vor `device_data->mutex`) gecacht (`swapchain_runtime_info`), sodass kein MCP-Handler mehr eine `IDXGISwapChain`-Methode unter `device_data->mutex` aufruft. Zusätzlich geben `BuildUpgradeAnalysisInputForMcp` und `ListTrackedResourcesForMcp` `device_data->mutex` vor `ForEachResourceInfo` frei.
- **`devkit_check_upgrade_candidates` (Vulkan):** Nicht-schreibbare Sampled-Texturen ohne `render_target`/`unordered_access`/`copy_dest` werden ausgefiltert und `usage_include` strikt aus den echten `desc.usage`-Flags abgeleitet.
