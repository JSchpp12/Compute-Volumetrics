import os
import argparse
import json
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from tqdm import tqdm


def gather_json_files(root_dir: Path) -> list[Path]:
    """Recursively gather all .json file paths from a directory."""
    return [
        Path(os.path.join(dirpath, filename))
        for dirpath, _, filenames in os.walk(root_dir)
        for filename in filenames
        if filename.endswith(".json")
    ]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Add volume_info field to image metric JSON files from a scene file."
    )
    parser.add_argument(
        "root_dir", type=str, help="Path to the root directory to search."
    )
    parser.add_argument(
        "scene_file", type=str, help="Path to the scene file used to extract volume orientation."
    )
    return parser.parse_args()


def process_file(file_path: Path, scene_objects: dict) -> None:
    with open(file_path, "r") as f:
        data = json.load(f)

    fog_volume_name = data.get("fog_volume_name")
    if not fog_volume_name:
        return

    volume_obj = scene_objects.get(fog_volume_name)
    if not volume_obj:
        return

    if "volume_info" in data:
        return

    volume_info = {
        "position": volume_obj["position"],
        "rotation": volume_obj["rotation_deg"],
        "scale": volume_obj["scale"],
    }

    data["volume_info"] = volume_info

    with open(file_path, "w") as f:
        json.dump(data, f, indent=4)


def main():
    args = parse_args()
    root_dir = Path(args.root_dir)
    scene_path = Path(args.scene_file)

    with open(scene_path, "r") as f:
        scene_data = json.load(f)

    scene_objects = scene_data["Scene"]["Objects"]
    json_files = gather_json_files(root_dir)

    with tqdm(total=len(json_files), desc="Processing files", unit="file") as bar:
        with ThreadPoolExecutor(max_workers=16) as executor:
            futures = {executor.submit(process_file, fp, scene_objects): fp for fp in json_files}
            for future in as_completed(futures):
                try:
                    future.result()
                except Exception as e:
                    print(f"Error processing {futures[future]}: {e}")
                finally:
                    bar.update(1)


if __name__ == "__main__":
    main()
