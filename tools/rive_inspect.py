#!/usr/bin/env python3

# ==============================================================================
#
#  This file is part of the YUP library.
#  Copyright (c) 2026 - kunitoki@gmail.com
#
#  YUP is an open source library subject to open-source licensing.
#
#  The code included in this file is provided under the terms of the ISC license
#  http://www.isc.org/downloads/software-support-policy/isc-license. Permission
#  to use, copy, modify, and/or distribute this software for any purpose with or
#  without fee is hereby granted provided that the above copyright notice and
#  this permission notice appear in all copies.
#
#  YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
#  EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
#  DISCLAIMED.
#
# ==============================================================================

"""Inspect the contents of a Rive (.riv) runtime file.

Parses the Rive v7 binary format directly (no rive runtime needed) and lets you
inspect the object table of a .riv file:

    rive_inspect.py alien.riv names
    rive_inspect.py alien.riv names --json
    rive_inspect.py alien.riv info "Pata01"
    rive_inspect.py alien.riv info 150
    rive_inspect.py alien.riv tree
    rive_inspect.py alien.riv tree --node "Pata01"

Property keys and object type keys are resolved against the vendored rive
generated headers (thirdparty/rive/include/rive/generated), which is also how
the rive runtime itself decodes the file. Object hierarchy follows the runtime's
own resolution: parentId is an index into the owning artboard's component list
(components in file order, the artboard itself at index 0), not the raw file
position.
"""

from __future__ import annotations

import argparse
import glob
import json
import re
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
DEFAULT_RIVE_INCLUDE = REPO_ROOT / "thirdparty" / "rive" / "include" / "rive"

# Core*Type::id values, matching the vendored rive field types.
TYPE_UINT, TYPE_STRING, TYPE_DOUBLE, TYPE_COLOR, TYPE_BOOL, TYPE_BYTES = 0, 1, 2, 3, 4, 5
TYPE_NAMES = {TYPE_UINT: "uint", TYPE_STRING: "string", TYPE_DOUBLE: "double", TYPE_COLOR: "color", TYPE_BOOL: "bool", TYPE_BYTES: "bytes"}

# -----------------------------------------------------------------------------
# Binary readers (LEB128 varints, little-endian values).


def read_varuint(data: bytes, pos: int) -> tuple[int, int]:
    result = 0
    shift = 0
    while True:
        byte = data[pos]
        pos += 1
        result |= (byte & 0x7F) << shift
        if not (byte & 0x80):
            return result, pos
        shift += 7


def read_f32(data: bytes, pos: int) -> tuple[float, int]:
    return struct.unpack_from("<f", data, pos)[0], pos + 4


def read_u32(data: bytes, pos: int) -> tuple[int, int]:
    return struct.unpack_from("<I", data, pos)[0], pos + 4


def read_string(data: bytes, pos: int) -> tuple[str, int]:
    length, pos = read_varuint(data, pos)
    return data[pos : pos + length].decode("utf-8", "replace"), pos + length


def skip_bytes(data: bytes, pos: int) -> int:
    length, pos = read_varuint(data, pos)
    return pos + length


# -----------------------------------------------------------------------------
# Schema built from the vendored rive generated headers.

PROPERTY_KEY_RE = re.compile(r"static const uint16_t (\w+)PropertyKey = (\d+);")
TYPE_KEY_RE = re.compile(r"static const uint16_t typeKey = (\d+);")
CLASS_RE = re.compile(r"class (\w+Base)\b")
ARTBOARD_TYPE_KEY = 1  # ArtboardBase::typeKey


@dataclass
class PropertySchema:
    """Decoding information extracted from the vendored rive generated headers."""

    property_names: dict[int, str]  # property key -> name (first class wins)
    property_types: dict[int, int]  # property key -> Core*Type id
    type_names: dict[int, str]  # object type key -> class name
    component_keys: set[int]  # object type keys that derive from ComponentBase
    parent_key: int  # property key of the parent index
    name_key: int  # property key of the object name


def build_property_schema(rive_include: Path) -> PropertySchema:
    generated_dir = rive_include / "generated"
    if not generated_dir.is_dir():
        raise SystemExit(f"Rive generated headers not found under {generated_dir}")

    property_names: dict[int, str] = {}
    class_props: dict[str, dict[str, int]] = {}
    type_names: dict[int, str] = {}
    component_keys: set[int] = set()

    for header in sorted(glob.glob(str(generated_dir / "**" / "*_base.hpp"), recursive=True)):
        text = Path(header).read_text(encoding="utf-8")

        class_match = CLASS_RE.search(text)
        if class_match is None:
            continue
        class_name = class_match.group(1)[: -len("Base")]

        for key_match in PROPERTY_KEY_RE.finditer(text):
            name, key = key_match.group(1), int(key_match.group(2))
            class_props.setdefault(class_name, {})[name] = key
            property_names.setdefault(key, name)

        type_match = TYPE_KEY_RE.search(text)
        if type_match is None:
            continue
        type_key = int(type_match.group(1))
        type_names.setdefault(type_key, class_name)

        # ComponentBase::typeKey (10) listed in isTypeOf means the class derives
        # from ComponentBase, i.e. it occupies an artboard component slot.
        if re.search(r"case ComponentBase::typeKey:", text):
            component_keys.add(type_key)

    # Property key -> Core*Type id from CoreRegistry::propertyFieldId.
    property_types: dict[int, int] = {}
    registry_path = generated_dir / "core_registry.hpp"
    registry_text = registry_path.read_text(encoding="utf-8")
    function_match = re.search(
        r"static int propertyFieldId\(int propertyKey\)\s*\{(.*?)\n    \}",
        registry_text,
        re.DOTALL,
    )
    if function_match is None:
        raise SystemExit(f"Could not locate CoreRegistry::propertyFieldId in {registry_path}")

    # re.split keeps the case-group text BEFORE each "return <Type>::id;".
    parts = re.split(r"return (\w+)::id;", function_match.group(1))
    type_ids = {"CoreUintType": TYPE_UINT, "CoreStringType": TYPE_STRING, "CoreDoubleType": TYPE_DOUBLE, "CoreColorType": TYPE_COLOR, "CoreBoolType": TYPE_BOOL}
    for index in range(1, len(parts), 2):
        type_id = type_ids.get(parts[index])
        if type_id is None:
            continue
        for case_match in re.finditer(r"case (\w+)::(\w+)PropertyKey:", parts[index - 1]):
            class_name, prop_name = case_match.group(1), case_match.group(2)
            if class_name.endswith("Base"):
                class_name = class_name[: -len("Base")]
            key = class_props.get(class_name, {}).get(prop_name)
            if key is not None:
                property_types[key] = type_id

    # Properties stored as raw bytes, only handled by their owning class.
    for header in sorted(glob.glob(str(generated_dir / "**" / "*_base.hpp"), recursive=True)):
        text = Path(header).read_text(encoding="utf-8")
        for match in re.finditer(r"case (\w+)PropertyKey:[\s\S]{0,300}?CoreBytesType::deserialize\(reader\);", text):
            key = property_names.get(match.group(1))
            if key is not None:
                property_types[key] = TYPE_BYTES

    return PropertySchema(
        property_names=property_names,
        property_types=property_types,
        type_names=type_names,
        component_keys=component_keys,
        parent_key=class_props.get("Component", {}).get("parentId", 5),
        name_key=class_props.get("Component", {}).get("name", 4),
    )


# -----------------------------------------------------------------------------
# Rive file parsing.


@dataclass
class RivObject:
    index: int
    type_key: int
    props: dict[int, Any]

    def name(self, schema: PropertySchema) -> str | None:
        value = self.props.get(schema.name_key)
        return value if isinstance(value, str) else None


class RivFile:
    """A parsed Rive runtime file."""

    def __init__(self, path: Path, schema: PropertySchema):
        self.path = path
        self.schema = schema
        self.major: int = 0
        self.minor: int = 0
        self.file_id: int = 0
        self.objects: list[RivObject] = []
        self._parent: dict[int, int | None] = {}
        self._parse()

    def _parse(self) -> None:
        data = self.path.read_bytes()

        if data[:4] != b"RIVE":
            raise SystemExit(f"{self.path} is not a Rive file (missing 'RIVE' magic)")

        pos = 4
        self.major, pos = read_varuint(data, pos)
        self.minor, pos = read_varuint(data, pos)
        self.file_id, pos = read_varuint(data, pos)

        # File table-of-contents: property keys this runtime version may not know,
        # packed as 2-bit field indices (4 per uint32).
        toc_keys: list[int] = []
        while True:
            key, pos = read_varuint(data, pos)
            if key == 0:
                break
            toc_keys.append(key)

        toc_types: dict[int, int] = {}
        current, bit = 0, 8
        for index, key in enumerate(toc_keys):
            if bit == 8:
                current, pos = read_u32(data, pos)
                bit = 0
            toc_types[key] = (current >> bit) & 3
            bit += 2

        schema = self.schema
        objects: list[RivObject] = []

        # parentId resolves into the owning artboard's component list: the
        # artboard itself is slot 0, followed by every component (or null object)
        # in file order until the next artboard, matching Artboard::m_Objects.
        slot_to_file: list[dict[int, int]] = []
        object_slot: list[int | None] = []
        object_artboard: list[int | None] = []

        while pos < len(data):
            file_index = len(objects)
            type_key, pos = read_varuint(data, pos)
            props: dict[int, Any] = {}
            while True:
                prop_key, pos = read_varuint(data, pos)
                if prop_key == 0:
                    break

                type_id = schema.property_types.get(prop_key)
                if type_id is None:
                    # Unknown to this runtime: fall back to the file's own TOC.
                    type_id = {0: TYPE_UINT, 1: TYPE_STRING, 2: TYPE_DOUBLE, 3: TYPE_COLOR}[toc_types.get(prop_key, 0)]

                if type_id == TYPE_UINT:
                    value, pos = read_varuint(data, pos)
                elif type_id == TYPE_STRING:
                    value, pos = read_string(data, pos)
                elif type_id == TYPE_DOUBLE:
                    value, pos = read_f32(data, pos)
                elif type_id == TYPE_COLOR:
                    value, pos = read_u32(data, pos)
                elif type_id == TYPE_BOOL:
                    value = data[pos] == 1
                    pos += 1
                else:
                    start = pos
                    pos = skip_bytes(data, pos)
                    value = f"<bytes: {pos - start}>"

                props[prop_key] = value

            objects.append(RivObject(file_index, type_key, props))

            if type_key == ARTBOARD_TYPE_KEY:
                # The artboard is always the first object of its component list.
                slot_to_file.append({0: file_index})
                object_artboard.append(len(slot_to_file) - 1)
                object_slot.append(0)
            elif slot_to_file and (type_key in schema.component_keys or type_key not in schema.type_names):
                # Components (and unknown/null objects) occupy the next slot of
                # the current artboard; animations, keyframes, assets, etc. do not.
                artboard = len(slot_to_file) - 1
                slot = len(slot_to_file[artboard])
                slot_to_file[artboard][slot] = file_index
                object_artboard.append(artboard)
                object_slot.append(slot)
            else:
                object_artboard.append(None)
                object_slot.append(None)

        # Resolve parentId through each artboard's slot map.
        self._parent = {}
        for file_index, obj in enumerate(objects):
            parent_slot = obj.props.get(schema.parent_key)
            artboard = object_artboard[file_index]
            if parent_slot is None or artboard is None:
                self._parent[file_index] = None
                continue
            self._parent[file_index] = slot_to_file[artboard].get(parent_slot)

        self.objects = objects

    def type_name(self, type_key: int) -> str:
        return self.schema.type_names.get(type_key, f"Type{type_key}")

    def find(self, name_or_index: str | int) -> RivObject | None:
        if isinstance(name_or_index, int) or str(name_or_index).isdigit():
            index = int(name_or_index)
            if 0 <= index < len(self.objects):
                return self.objects[index]
        for obj in self.objects:
            if obj.name(self.schema) == name_or_index:
                return obj
        return None

    def parent_of(self, obj: RivObject) -> RivObject | None:
        parent_index = self._parent.get(obj.index)
        if parent_index is None:
            return None
        return self.objects[parent_index]

    def children(self, obj: RivObject) -> list[RivObject]:
        return [candidate for candidate in self.objects if self._parent.get(candidate.index) == obj.index]

    def label(self, obj: RivObject) -> str:
        name = obj.name(self.schema)
        return f"{obj.index} [{self.type_name(obj.type_key)}] {name}" if name else f"{obj.index} [{self.type_name(obj.type_key)}]"


# -----------------------------------------------------------------------------
# Output helpers.


def _format_value(value: Any, type_id: int | None) -> str:
    if type_id == TYPE_COLOR and isinstance(value, int):
        return f"0x{value:08x}"
    if isinstance(value, str):
        return f'"{value}"'
    if isinstance(value, float):
        return f"{value:g}"
    return str(value)


# -----------------------------------------------------------------------------
# Commands.


def cmd_names(riv: RivFile, args: argparse.Namespace) -> int:
    schema = riv.schema
    entries = []
    for obj in riv.objects:
        name = obj.name(schema)
        if not name:
            continue
        parent = riv.parent_of(obj)
        entry = {
            "index": obj.index,
            "type": riv.type_name(obj.type_key),
            "typeKey": obj.type_key,
            "parent": parent.index if parent else None,
            "parentName": parent.name(schema) if parent else None,
            "name": name,
        }
        entries.append(entry)

    if args.json:
        print(json.dumps(entries, indent=2))
        return 0

    width = max(len(str(entry["index"])) for entry in entries) if entries else 1
    for entry in entries:
        parent = f"{entry['parent']} {entry['parentName']}".strip() if entry["parent"] is not None else "-"
        print(f"{entry['index']:>{width}d}  {entry['type']:<22}  parent={parent:<24}  {entry['name']}")
    print(f"\n{len(entries)} named object(s) in {riv.path}")
    return 0


def cmd_info(riv: RivFile, args: argparse.Namespace) -> int:
    target = riv.find(args.node)
    if target is None:
        raise SystemExit(f"Object '{args.node}' not found")

    schema = riv.schema
    parent = riv.parent_of(target)
    info = {
        "index": target.index,
        "type": riv.type_name(target.type_key),
        "typeKey": target.type_key,
        "parent": parent.index if parent else None,
        "parentName": parent.name(schema) if parent else None,
    }

    if args.json:
        props = {schema.property_names.get(key, str(key)): value for key, value in target.props.items()}
        print(json.dumps({**info, "properties": props}, indent=2))
        return 0

    print(f"object {target.index} [{riv.type_name(target.type_key)}] (typeKey {target.type_key})")
    if parent:
        parent_name = parent.name(schema)
        print(f"parent: {parent.index}{f' ({parent_name})' if parent_name else ''}")
    if not target.props:
        print("(no properties)")
    for key, value in target.props.items():
        name = schema.property_names.get(key, f"key{key}")
        type_id = schema.property_types.get(key)
        type_suffix = f" ({TYPE_NAMES.get(type_id, '?')})" if type_id is not None else ""
        print(f"  {name}{type_suffix} = {_format_value(value, type_id)}")
    return 0


def cmd_tree(riv: RivFile, args: argparse.Namespace) -> int:
    root = riv.find(args.node) if args.node else None

    def print_subtree(obj: RivObject, indent: str, is_last: bool) -> None:
        connector = "`- " if is_last else "|- "
        print(f"{indent}{connector}{riv.label(obj)}")
        children = riv.children(obj)
        child_indent = indent + ("   " if is_last else "|  ")
        for index, child in enumerate(children):
            print_subtree(child, child_indent, index == len(children) - 1)

    if root is not None:
        print(f"subtree rooted at {riv.label(root)}")
        children = riv.children(root)
        for index, child in enumerate(children):
            print_subtree(child, "", index == len(children) - 1)
        return 0

    roots = [obj for obj in riv.objects if riv.parent_of(obj) is None]
    for index, obj in enumerate(roots):
        print(riv.label(obj))
        children = riv.children(obj)
        for child_index, child in enumerate(children):
            print_subtree(child, "", child_index == len(children) - 1)
    return 0


# -----------------------------------------------------------------------------
# CLI.


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Inspect the contents of a Rive (.riv) runtime file.")
    parser.add_argument("file", help="Path to the .riv file to inspect.")
    parser.add_argument(
        "--rive-include",
        default=str(DEFAULT_RIVE_INCLUDE),
        help="Path to the vendored rive include directory used to resolve keys (default: %(default)s).",
    )

    subparsers = parser.add_subparsers(dest="command", required=True)

    names = subparsers.add_parser("names", help="List all named objects.")
    names.add_argument("--json", action="store_true", help="Emit a JSON array instead of a table.")
    names.set_defaults(func=cmd_names)

    info = subparsers.add_parser("info", help="Dump the properties of one object, by name or index.")
    info.add_argument("node", help="Object name or index.")
    info.add_argument("--json", action="store_true", help="Emit JSON instead of a table.")
    info.set_defaults(func=cmd_info)

    tree = subparsers.add_parser("tree", help="Print the object hierarchy.")
    tree.add_argument("--node", help="Root the tree at this object (name or index) instead of the file roots.")
    tree.set_defaults(func=cmd_tree)

    return parser.parse_args()


def main() -> int:
    args = parse_args()
    schema = build_property_schema(Path(args.rive_include))
    riv = RivFile(Path(args.file), schema)
    try:
        return args.func(riv, args)
    except BrokenPipeError:
        # Piping into tools like `head` closes stdout early; exit quietly.
        try:
            sys.stdout.close()
        except BrokenPipeError:
            pass
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
