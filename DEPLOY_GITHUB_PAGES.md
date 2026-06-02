# Deploying Sudoku Reasoning Radar to GitHub Pages

This guide prepares the static website for GitHub Pages. It does not require sharing any password, token, cookie, or secret with Codex or any prompt.

## Important Safety Notes

- Do not give your GitHub password or token to Codex or any prompt.
- Do not commit secrets, cookies, tokens, SSH keys, or private personal files.
- GitHub Pages is public by default.
- Review release ZIP contents before publishing.
- If you do not want the Windows ZIP inside the repository, upload it to GitHub Releases and update the download link.

## Recommended Project Layout

```text
D:\Soduku
website\                 Source website files
docs\                    GitHub Pages publish folder
package_windows.bat      Creates Windows ZIP release
sync_website_to_docs.bat Copies website/ to docs/
preview_website.bat      Opens local website preview
```

GitHub Pages can publish from the `main` branch and `/docs` folder.

## Create A GitHub Repository

1. Open GitHub in your browser.
2. Create a new repository, for example:

```text
sudoku-reasoning-radar
```

3. Do not give your password, token, or cookie to Codex.

## Command Line Publishing

From `D:\Soduku`:

```bat
git init
git add .
git commit -m "Initial Sudoku Reasoning Radar website"
git remote add origin YOUR_REPOSITORY_URL_HERE
git branch -M main
git push -u origin main
```

Replace `YOUR_REPOSITORY_URL_HERE` with the repository URL copied from GitHub.

## Enable GitHub Pages

1. Open the repository page on GitHub.
2. Open Settings.
3. Open Pages.
4. Under Source, choose:

```text
Deploy from a branch
```

5. Set Branch to:

```text
main
```

6. Set Folder to:

```text
/docs
```

7. Save.
8. Wait for GitHub Pages to build.
9. The public website URL will appear in the Pages settings page.

## GitHub Desktop Option

If you prefer GitHub Desktop:

1. Open GitHub Desktop.
2. Choose Add local repository.
3. Select `D:\Soduku`.
4. Publish repository.
5. Push changes.
6. Open GitHub repository settings in the browser.
7. Configure Pages with `main` and `/docs`.

## Updating The Website

Edit files in:

```text
website\
```

Then run:

```bat
sync_website_to_docs.bat
```

Commit and push the updated files.

## Download Link Placeholder

The download buttons currently point to:

```text
downloads/SudokuReasoningRadar_Windows.zip
```

You can provide this ZIP in either of two ways:

1. Commit the ZIP into `docs/downloads/`.
2. Upload the ZIP to GitHub Releases and replace the download link with the GitHub Release asset URL.

The GitHub link in the website currently uses:

```text
#github-link-placeholder
```

Replace it with the real repository URL after the repository exists.
