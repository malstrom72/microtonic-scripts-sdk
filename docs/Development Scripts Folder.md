# Development Scripts Folder

For quicker round-trips during development, keep a project-local copy of the entire `Microtonic Scripts` folder and
link Microtonic's scripts folder to it. Copy the current scripts folder into your project first so existing scripts are
preserved. This pairs well with the [live scripting bridge](../README.md#live-scripting-bridge): edit a file, then
reload over the bridge with `mt_reload`, passing an `until` expression that observes the edited code.

On macOS, Microtonic's scripts folder is normally:

```text
/Library/Application Support/Sonic Charge/Microtonic Scripts/
```

Copy it into your project, move the original aside, then create a symbolic link:

```sh
mkdir -p "/path/to/my-microtonic-scripts"
cp -R "/Library/Application Support/Sonic Charge/Microtonic Scripts" \
  "/path/to/my-microtonic-scripts/scripts"
sudo mv "/Library/Application Support/Sonic Charge/Microtonic Scripts" \
  "/Library/Application Support/Sonic Charge/Microtonic Scripts.backup"
sudo ln -s "/path/to/my-microtonic-scripts/scripts" \
  "/Library/Application Support/Sonic Charge/Microtonic Scripts"
```

If an interactive `sudo` password prompt is inconvenient, macOS' native administrator dialog can run
the privileged link step instead. After copying the current folder into the project and moving the
original aside, run:

```sh
osascript -e 'do shell script "rm -f \"/Library/Application Support/Sonic Charge/Microtonic Scripts\" && ln -s \"/path/to/my-microtonic-scripts/scripts\" \"/Library/Application Support/Sonic Charge/Microtonic Scripts\"" with administrator privileges'
```

The `with administrator privileges` clause runs the whole shell script as root from one GUI prompt,
so both the `rm` and `ln -s` operations are covered.

**Cold start (the folder does not exist yet).** On a never-used Microtonic install there is no
`Microtonic Scripts` folder — Microtonic shows the puzzle (script) menu but keeps it greyed out and
inactive until the folder exists — so there is nothing to copy or back up. Skip the `cp -R` and
`mv ... .backup` steps entirely: create your project `scripts/` folder and link the standard location
straight to it. There is also no `rm` to do, since nothing is there to remove:

```sh
mkdir -p "/path/to/my-microtonic-scripts/scripts"
osascript -e "do shell script \"ln -s '/path/to/my-microtonic-scripts/scripts' '/Library/Application Support/Sonic Charge/Microtonic Scripts'\" with administrator privileges"
```

Note the `-e` argument is double-quoted so your shell expands the project path before `osascript`
sees it, while the paths inside use single quotes so their spaces survive. Prefer a project **outside**
`~/Documents`/Desktop/Downloads: Microtonic reading scripts through a link into those folders can raise
a one-time "wants to access Documents" prompt.

If `ls -ld` or `readlink` shows that the exact live folder opened by Microtonic is already a symlink
to another workspace, do not repeat the `mv ... .backup` step. The original folder was already
preserved during the first setup. Re-link by removing the existing symlink and creating the new one;
`rm` on a symlink removes only the link, never its target.

On Windows, use `Open Scripts Folder` in Microtonic to confirm the exact folder. The common location is:

```text
%PROGRAMFILES%\Sonic Charge\Microtonic Scripts
```

Before the bridge is installed, the SDK helper can usually locate the same folder from the Sonic Charge registry keys
(it reads `SetupPath` from `HKLM\SOFTWARE\Sonic Charge\Microtonic` and appends `Microtonic Scripts`):

```powershell
powershell -ExecutionPolicy Bypass -File tools\locate-scripts-folder.ps1 -Verify
```

Treat that output as a candidate to confirm, not as a substitute for the exact folder opened by Microtonic. If the
registry read fails, the engine may fall back to the plugin binary directory, which the helper cannot know.

The live folder may require elevation to modify; a one-time junction avoids repeated elevated copies while iterating.
Copy it into your project, move the original aside, then create a directory junction:

```bat
xcopy "%PROGRAMFILES%\Sonic Charge\Microtonic Scripts" ^
  "C:\path\to\my-microtonic-scripts\scripts\" /E /I
ren "%PROGRAMFILES%\Sonic Charge\Microtonic Scripts" "Microtonic Scripts.backup"
mklink /J "%PROGRAMFILES%\Sonic Charge\Microtonic Scripts" ^
  "C:\path\to\my-microtonic-scripts\scripts"
```

After linking, scripts edited in your project-local `scripts` directory are the same scripts Microtonic sees.
