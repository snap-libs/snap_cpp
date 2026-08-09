import os
import shutil
import subprocess
import sys

def main():
    base_dir = r"c:\work\snap"
    website_dir = r"c:\work\website"
    target_repo_dir = os.path.join(base_dir, "scratch", "snap_repo")
    repo_url = "https://github.com/snap-libs/snap.git"

    print(f"[1/4] Checking target repository directory: {target_repo_dir}")
    if not os.path.exists(target_repo_dir):
        print(f"Cloning {repo_url}...")
        os.makedirs(os.path.dirname(target_repo_dir), exist_ok=True)
        subprocess.run(["git", "clone", repo_url, target_repo_dir], check=True)
    else:
        print("Fetching latest status from origin/main...")
        subprocess.run(["git", "pull", "origin", "main"], cwd=target_repo_dir, check=False)

    print("[2/4] Syncing website/ files to target repository...")
    valid_extensions = {".html", ".css", ".js", ".png", ".jpg", ".jpeg", ".svg", ".wav", ".mp4"}
    
    for root, dirs, files in os.walk(website_dir):
        rel_path = os.path.relpath(root, website_dir)
        dest_dir = target_repo_dir if rel_path == "." else os.path.join(target_repo_dir, rel_path)
        os.makedirs(dest_dir, exist_ok=True)
        for f in files:
            ext = os.path.splitext(f)[1].lower()
            if ext in valid_extensions or f == ".nojekyll":
                src_file = os.path.join(root, f)
                dest_file = os.path.join(dest_dir, f)
                shutil.copy2(src_file, dest_file)

    nojekyll = os.path.join(target_repo_dir, ".nojekyll")
    if not os.path.exists(nojekyll):
        with open(nojekyll, "w", encoding="utf-8") as fp:
            fp.write("# Prevent GitHub Pages Jekyll build\n")

    print("[3/4] Checking git status in target repo...")
    status = subprocess.run(["git", "status", "--porcelain"], cwd=target_repo_dir, capture_output=True, text=True)
    if not status.stdout.strip():
        print("No changes detected in website files. Deployment skipped.")
        return

    print("[4/4] Committing and pushing website updates...")
    subprocess.run(["git", "add", "."], cwd=target_repo_dir, check=True)
    commit_msg = sys.argv[1] if len(sys.argv) > 1 else "docs: update SNAP website landing assets"
    subprocess.run(["git", "commit", "-m", commit_msg], cwd=target_repo_dir, check=True)
    subprocess.run(["git", "push", "origin", "main"], cwd=target_repo_dir, check=True)
    print("✅ Successfully deployed website updates to https://github.com/snap-libs/snap!")

if __name__ == "__main__":
    main()
