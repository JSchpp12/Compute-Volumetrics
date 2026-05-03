import os
import argparse
import json
from tqdm import tqdm
from dataclasses import dataclass, asdict
from typing import Optional

from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed


@dataclass
class Vec3:
    x: float
    y: float
    z: float


@dataclass
class Vec2:
    x: float
    y: float


@dataclass
class TerrainShapeFile:
    center: Vec2
    view_distance: int

    @classmethod
    def from_dict(cls, data: dict) -> "TerrainShapeFile":
        center: Optional[Vec2] = None
        if "lat" in data["center"]:
            center = Vec2(data["center"]["lat"], data["center"]["lon"])
        else:
            center = Vec2(**data["center"])

        if "range" in data:
            vis_range = data["range"]
        else:
            vis_range = data["view_distance"]
        return cls(
            center,
            view_distance=vis_range,
        )

    @classmethod
    def from_json(cls, path: Path) -> "TerrainShapeFile":
        with open(path, "r") as f:
            data = json.load(f)
            return cls.from_dict(data)

    def to_json(self) -> str:
        return json.dumps(asdict(self), indent=4)


@dataclass
class ExpFogInfo:
    density: float


@dataclass
class HomogenousInfo:
    maxNumSteps: int


@dataclass
class LinearInfo:
    farDist: float
    nearDist: float


@dataclass
class MarchedInfo:
    defaultDensity: float
    densityMultiplier: float
    lightPropertyDirG: float
    sigmaAbsorption: float
    sigmaScattering: float
    stepSizeDist: float
    stepSizeDist_light: float
    cutoffValue: Optional[float]


@dataclass
class FogParams:
    expFogInfo: ExpFogInfo
    homogenousInfo: HomogenousInfo
    linearInfo: LinearInfo
    marchedInfo: MarchedInfo

    @classmethod
    def from_dict(cls, data: dict) -> "FogParams":
        marched = None
        if "cutoffValue" not in data["marchedInfo"]:
            data["cutoffValue"] = 0.001

        return cls(
            expFogInfo=ExpFogInfo(**data["expFogInfo"]),
            homogenousInfo=HomogenousInfo(**data["homogenousInfo"]),
            linearInfo=LinearInfo(**data["linearInfo"]),
            marchedInfo=MarchedInfo(**data["marchedInfo"]),
        )


@dataclass
class Frame:
    camera_look_dir: Vec3
    camera_position: Vec3
    file_name: str
    fog_params: FogParams
    fog_type: str
    visibility_distance: float
    terrain_shape: Optional[TerrainShapeFile]
    terrain_name: Optional[str]
    terrain_shape_type: str

    @classmethod
    def from_dict(cls, data: dict) -> "Frame":
        tName: Optional[str] = None
        if "terrain_name" in data:
            tName = str(data["terrain_name"])

        tData: Optional[TerrainShapeFile] = None
        if "terrain_shape" in data:
            tData = TerrainShapeFile.from_dict(data["terrain_shape"])

        if "terrain_shape_type" in data:
            tShapeType = data["terrain_shape_type"]
        else:
            tShapeType = "real"

        return cls(
            camera_look_dir=Vec3(**data["camera_look_dir"]),
            camera_position=Vec3(**data["camera_position"]),
            file_name=data["file_name"],
            fog_params=FogParams.from_dict(data["fog_params"]),
            fog_type=data["fog_type"],
            visibility_distance=data["visibility_distance"],
            terrain_shape=tData,
            terrain_name=tName,
            terrain_shape_type=tShapeType,
        )

    @classmethod
    def from_json(cls, path: Path) -> "Frame":
        with open(path, "r") as f:
            return cls.from_dict(json.load(f))

    def to_json(self, path: Path) -> None:
        with open(path, "w") as f:
            json.dump(asdict(self), f, indent=4)


@dataclass
class ImageToUpdate:
    path: Path
    override_shape_info: Optional[TerrainShapeFile]
    override_name: Optional[str]


def gather_image_metric_files(root_dir: Path) -> list[ImageToUpdate]:
    """Recursively gather all .json file paths from a directory."""
    return [
        ImageToUpdate(Path(os.path.join(dirpath, filename)), None, None)
        for dirpath, _, filenames in os.walk(root_dir)
        for filename in filenames
        if filename.endswith(".json")
    ]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Find all JSON files in a directory.")
    parser.add_argument(
        "root_dir", type=str, help="Path to the root directory to search."
    )

    return parser.parse_args()


def process_image_file(img_data: ImageToUpdate) -> None:
    data = Frame.from_json(img_data.path)

    data.to_json(img_data.path)


def main():
    """_summary_
    Corrects the following in image metric files:
    Earlier metric files do not have the terrain_shape parameter in the data.
    Or it might be incorrect/null. This scripts takes in a path to the root directory to patch
    and a shape json file to manually write into the files.
    """

    args = parse_args()
    metric_files: list[ImageToUpdate] = gather_image_metric_files(args.root_dir)

    with tqdm(total=len(metric_files), desc="Processing files", unit="file") as bar:
        with ThreadPoolExecutor(max_workers=16) as executor:
            futures = {executor.submit(process_image_file, f): f for f in metric_files}
            for future in as_completed(futures):
                try:
                    future.result()
                except Exception as e:
                    print(f"Error processing {futures[future].path}: {e}")
                finally:
                    bar.update(1)


if __name__ == "__main__":
    main()
