# SISSHIN HZ MACHINE — build a Windows VST3 with no local compiling

1. Create a free GitHub account and a new repository (private is fine).
2. Upload everything in this folder, keeping the folder structure
   (including the hidden `.github/workflows/build.yml`). Dragging the unzipped folder
   into the "Add file → Upload files" page works.
3. Open the repository's **Actions** tab. The "Build VST3" workflow starts automatically
   (or press "Run workflow"). The first build takes roughly 10–20 minutes.
4. When it finishes, open the run and download the **SISSHIN-HZ-MACHINE-VST3** artifact.
5. Unzip it and copy `SISSHIN HZ MACHINE.vst3` to `C:\Program Files\Common Files\VST3\`.
6. In FL Studio: Options → Manage plugins → Find plugins.

If the build fails, open the failed step, copy the red error lines, and paste them to Claude.
The code has not been compiled yet, so expect one or two small fixes on the first run.

## Requirements on the user's PC
Windows 10/11 with the Microsoft Edge WebView2 Runtime (preinstalled on Windows 11
and on up-to-date Windows 10).

## Layout
- `web/index.html`        the GUI (also opens in a normal browser for a UI-only preview)
- `Source/DSP/`           AmpModel interface, HZMachineDSP (conventional DSP), neural stub
- `Source/Params.h`       every parameter in one table
- `Source/PluginProcessor` audio, state saving, cabinet convolution, output stage
- `Source/PluginEditor`   hosts the web UI in a WebView and bridges parameters

## Editing the look
Edit `web/index.html`, commit, and the next build embeds it.
