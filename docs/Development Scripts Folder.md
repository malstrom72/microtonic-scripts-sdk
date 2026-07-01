# Development Scripts Folder

For quicker round-trips during development, keep a project-local copy of the entire `Microtonic Scripts` folder and
link Microtonic's scripts folder to it. Copy the current scripts folder into your project first so existing scripts are
preserved.

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

If `ls -ld` or `readlink` shows that the exact live folder opened by Microtonic is already a symlink
to another workspace, do not repeat the `mv ... .backup` step. The original folder was already
preserved during the first setup. Re-link by removing the existing symlink and creating the new one;
`rm` on a symlink removes only the link, never its target.

On Windows, use `Open Scripts Folder` in Microtonic to confirm the exact folder. The common location is:

```text
%PROGRAMFILES%\Sonic Charge\Microtonic Scripts
```

Copy it into your project, move the original aside, then create a directory junction:

```bat
xcopy "%PROGRAMFILES%\Sonic Charge\Microtonic Scripts" ^
  "C:\path\to\my-microtonic-scripts\scripts\" /E /I
ren "%PROGRAMFILES%\Sonic Charge\Microtonic Scripts" "Microtonic Scripts.backup"
mklink /J "%PROGRAMFILES%\Sonic Charge\Microtonic Scripts" ^
  "C:\path\to\my-microtonic-scripts\scripts"
```

After linking, scripts edited in your project-local `scripts` directory are the same scripts Microtonic sees.
