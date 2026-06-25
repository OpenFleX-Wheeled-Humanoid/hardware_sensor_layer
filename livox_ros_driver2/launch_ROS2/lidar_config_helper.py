#!/usr/bin/env python3
"""
Helper functions to merge user-level lidar config into MID360_config.json
"""
import json
import yaml
from pathlib import Path
from typing import Dict, Any


def load_user_lidar_config() -> Dict[str, Any]:
    """Load user lidar config from ~/.openflex/lidar_config.yaml"""
    user_config_path = Path.home() / ".openflex" / "lidar_config.yaml"

    if not user_config_path.exists():
        return {}

    try:
        with open(user_config_path, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
            return config if config else {}
    except Exception as e:
        print(f"Warning: Failed to load {user_config_path}: {e}")
        return {}


def merge_lidar_config(base_json_path: str) -> str:
    """
    Merge user lidar config into base MID360_config.json

    Args:
        base_json_path: Path to the base MID360_config.json

    Returns:
        Path to merged config (temporary file or original if no user config)
    """
    user_config = load_user_lidar_config()

    # If no user config, return original path
    if not user_config:
        return base_json_path

    # Load base JSON
    try:
        with open(base_json_path, 'r', encoding='utf-8') as f:
            base_config = json.load(f)
    except Exception as e:
        print(f"Error: Failed to load base config {base_json_path}: {e}")
        return base_json_path

    # Extract user IPs
    host_ip = user_config.get('host_network', {}).get('ip', None)
    lidar_ip = user_config.get('lidars', {}).get('mid360', {}).get('device_ip', None)

    # Merge host IP (4 places)
    if host_ip and 'Mid360s' in base_config and 'host_net_info' in base_config['Mid360s']:
        base_config['Mid360s']['host_net_info']['cmd_data_ip'] = host_ip
        base_config['Mid360s']['host_net_info']['point_data_ip'] = host_ip
        base_config['Mid360s']['host_net_info']['imu_data_ip'] = host_ip
        base_config['Mid360s']['host_net_info']['push_msg_ip'] = host_ip

    # Also check for "MID360" field (backward compatibility)
    if host_ip and 'MID360' in base_config and 'host_net_info' in base_config['MID360']:
        base_config['MID360']['host_net_info']['cmd_data_ip'] = host_ip
        base_config['MID360']['host_net_info']['point_data_ip'] = host_ip
        base_config['MID360']['host_net_info']['imu_data_ip'] = host_ip
        if 'push_msg_ip' in base_config['MID360']['host_net_info']:
            base_config['MID360']['host_net_info']['push_msg_ip'] = host_ip

    # Merge lidar IP
    if lidar_ip and 'lidar_configs' in base_config and len(base_config['lidar_configs']) > 0:
        base_config['lidar_configs'][0]['ip'] = lidar_ip

    # Write to temporary file with strict JSON formatting
    import tempfile
    fd, temp_path = tempfile.mkstemp(suffix='.json', prefix='mid360_config_')
    try:
        with open(temp_path, 'w', encoding='utf-8') as f:
            json.dump(base_config, f, indent=2, ensure_ascii=False)
        return temp_path
    except Exception as e:
        print(f"Error: Failed to write merged config: {e}")
        import os
        os.close(fd)
        os.unlink(temp_path)
        return base_json_path
