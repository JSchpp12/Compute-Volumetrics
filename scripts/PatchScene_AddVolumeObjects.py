"""Add one scene object per volume directory by duplicating an existing one.

The renderer resolves each object in Scene.Objects by looking up a volume
directory of the same name under media/volumes. This script takes a scene file
that already contains at least one <base>_<number> object (e.g. demo_1), uses it
as a template, and creates a copy of it for every <base>_<number> directory
found in media/volumes that the scene does not already contain.

Usage:
    python PatchScene_AddVolumeObjects.py scenes/alaska/demo/scene.json
    python PatchScene_AddVolumeObjects.py scenes/alaska/demo/scene.json --source demo_3
    python PatchScene_AddVolumeObjects.py scenes/alaska/demo/scene.json --volumes-dir media/volumes --dry-run
"""

import argparse
import copy
import json
import re
import sys
from pathlib import Path

NAME_PATTERN = re.compile(r"^(?P<base>.+)_(?P<number>\d+)$")


def number_key(name: str) -> tuple[str, int, str]:
    """Sort names with a numeric suffix by base name, then numerically."""
    match = NAME_PATTERN.match(name)
    if match is None:
        return (name, -1, name)
    return (match.group("base"), int(match.group("number")), name)


def base_name(name: str) -> str:
    match = NAME_PATTERN.match(name)
    return match.group("base") if match else name


def find_volumes_dir(scene_path: Path, explicit: str | None) -> Path:
    """Locate media/volumes, either from the CLI or by searching upwards."""
    if explicit is not None:
        volume_dir = Path(explicit)
        if not volume_dir.is_dir():
            sys.exit(f"error: volumes directory does not exist: {volume_dir}")
        return volume_dir

    for parent in scene_path.resolve().parents:
        candidate = parent / "media" / "volumes"
        if candidate.is_dir():
            return candidate

    repo_candidate = Path(__file__).resolve().parent.parent / "media" / "volumes"
    if repo_candidate.is_dir():
        return repo_candidate

    cwd_candidate = Path.cwd() / "media" / "volumes"
    if cwd_candidate.is_dir():
        return cwd_candidate

    sys.exit(
        "error: could not find media/volumes; pass it explicitly with --volumes-dir"
    )


def discover_volume_names(volume_dir: Path) -> list[str]:
    """Return volume directory names that carry a numeric suffix."""
    return sorted(
        (
            entry.name
            for entry in volume_dir.iterdir()
            if entry.is_dir() and NAME_PATTERN.match(entry.name)
        ),
        key=number_key,
    )


def discover_template_objects(objects: dict) -> list[str]:
    """Return existing scene objects whose names carry a numeric suffix."""
    return sorted(
        (name for name in objects if NAME_PATTERN.match(name)),
        key=number_key,
    )


def summarize(names: list[str], limit: int = 8) -> str:
    """Format a name list for an error message without dumping hundreds of lines."""
    if len(names) <= limit:
        return ", ".join(names) if names else "none"
    return f"{', '.join(names[:limit])}, ... ({len(names)} total)"


def choose_template(candidates: list[str], requested: str | None) -> str:
    """Pick the object to copy from, prompting when the choice is ambiguous."""
    if requested is not None:
        if requested not in candidates:
            sys.exit(
                f"error: source object '{requested}' not found "
                f"(available: {summarize(candidates)})"
            )
        return requested

    if len(candidates) == 1:
        return candidates[0]

    print("Multiple template candidates found:")
    for index, name in enumerate(candidates, start=1):
        print(f"  {index}. {name}")

    while True:
        try:
            answer = input(f"Select the source object [1-{len(candidates)}]: ").strip()
        except (EOFError, KeyboardInterrupt):
            sys.exit("\nAborted.")

        if not answer:
            continue
        try:
            choice = int(answer)
        except ValueError:
            print(f"Please enter a number between 1 and {len(candidates)}.")
            continue

        if 1 <= choice <= len(candidates):
            return candidates[choice - 1]

        print(f"Please enter a number between 1 and {len(candidates)}.")


def load_scene(scene_path: Path) -> dict:
    try:
        with open(scene_path, "r", encoding="utf-8") as f:
            return json.load(f)
    except FileNotFoundError:
        sys.exit(f"error: scene file does not exist: {scene_path}")
    except json.JSONDecodeError as exc:
        sys.exit(f"error: could not parse {scene_path}: {exc}")


def save_scene(scene_path: Path, data: dict) -> None:
    # The application serialises objects in alphabetical key order and writes
    # with two-space indentation and no trailing newline; match that layout so
    # the file reads as if the app itself had saved it.
    # newline="\n" keeps the file LF-terminated on Windows, matching the scenes
    # the application writes.
    with open(scene_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=2)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Duplicate an existing scene object into one object per volume "
            "directory found in media/volumes."
        )
    )
    parser.add_argument("scene_file", type=str, help="Path to the scene.json to modify.")
    parser.add_argument(
        "--volumes-dir",
        type=str,
        default=None,
        help="Path to the volumes directory (default: locate media/volumes).",
    )
    parser.add_argument(
        "--source",
        type=str,
        default=None,
        help="Name of the object to copy (skips the interactive prompt).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the objects that would be added without writing the file.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    scene_path = Path(args.scene_file)

    data = load_scene(scene_path)
    try:
        objects = data["Scene"]["Objects"]
    except (KeyError, TypeError):
        sys.exit(f"error: {scene_path} has no Scene.Objects section")

    candidates = discover_template_objects(objects)
    if not candidates:
        sys.exit(
            "error: no object with a numeric suffix (for example demo_1) exists in the scene"
        )

    volume_dir = find_volumes_dir(scene_path, args.volumes_dir)
    template_name = choose_template(candidates, args.source)
    template = objects[template_name]

    base = base_name(template_name)
    volume_names = [
        name for name in discover_volume_names(volume_dir) if base_name(name) == base
    ]

    if not volume_names:
        sys.exit(
            f"error: no volume directories matching '{base}_<number>' in {volume_dir}"
        )

    added: list[str] = []
    for volume in volume_names:
        if volume in objects:
            continue
        objects[volume] = copy.deepcopy(template)
        added.append(volume)

    if not added:
        print(f"No new objects to add; {template_name} already covers every volume.")
        return

    # Match the alphabetical ordering the application uses when it saves.
    data["Scene"]["Objects"] = dict(sorted(objects.items()))

    if args.dry_run:
        print(f"Would add {len(added)} object(s) copied from {template_name}:")
        for name in added:
            print(f"  {name}")
        return

    save_scene(scene_path, data)
    print(f"Added {len(added)} object(s) to {scene_path}, copied from {template_name}:")
    for name in added:
        print(f"  {name}")


if __name__ == "__main__":
    main()
