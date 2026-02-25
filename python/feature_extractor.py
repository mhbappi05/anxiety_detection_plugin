"""
Feature Extractor Module for Anxiety Detection
Extracts behavioral features from C/C++ programming sessions
"""

import numpy as np
from collections import deque
import re
from datetime import datetime, timedelta
from typing import List, Dict, Any, Tuple, Optional
import json

class FeatureExtractor:
    """Extracts features from programming session data for anxiety detection"""
    
    def __init__(self, window_size: int = 100):
        """
        Initialize feature extractor
        
        Args:
            window_size: Size of rolling window for real-time features
        """
        self.window_size = window_size
        self.rolling_keystrokes = deque(maxlen=window_size)
        self.rolling_compiles = deque(maxlen=window_size // 10)  # Store fewer compiles
        self.error_history = deque(maxlen=50)  # Store last 50 errors for RED metric
        
        # Error pattern normalization patterns
        self.normalization_patterns = [
            (r'line\s+\d+', 'line_num'),
            (r'column\s+\d+', 'col_num'),
            (r'[a-zA-Z]:\\(?:[^\\]+\\)*', 'path'),
            (r'0x[0-9a-f]+', 'memory_addr'),
            (r'\b\d+\b', 'number'),
            (r'\"[^\"]*\"', 'string_literal'),
            (r'\'[^\']*\'', 'char_literal')
        ]
        
        # C/C++ common error patterns for classification
        self.error_patterns = {
            'syntax_error': re.compile(r'syntax error|expected|unexpected', re.I),
            'missing_semicolon': re.compile(r'expected.*;|missing.*;', re.I),
            'undefined_reference': re.compile(r'undefined reference|unresolved external', re.I),
            'missing_header': re.compile(r'cannot find|no such file|include', re.I),
            'segmentation_fault': re.compile(r'segmentation fault|access violation', re.I),
            'null_pointer': re.compile(r'null pointer|nullptr|NULL', re.I),
            'array_bounds': re.compile(r'array bounds|out of bounds|index.*out of range', re.I),
            'uninitialized': re.compile(r'uninitialized|used without being initialized', re.I),
            'memory_leak': re.compile(r'memory leak|leaked', re.I),
            'buffer_overflow': re.compile(r'buffer overflow|stack overflow', re.I),
            'type_mismatch': re.compile(r'type mismatch|cannot convert|incompatible type', re.I),
            'no_matching_function': re.compile(r'no matching function|overload.*not found', re.I),
            'ambiguous': re.compile(r'ambiguous|more than one instance', re.I),
            'redefinition': re.compile(r'redefinition|multiple definition', re.I),
            'undeclared': re.compile(r'not declared|undeclared', re.I),
            'incomplete_type': re.compile(r'incomplete type|forward declaration', re.I)
        }
        
        # Baseline stats
        self.baseline = {
            'typing_speed': 40.0,  # Default 40 WPM
            'backspace_rate': 0.15,  # Default 15% backspaces
            'compile_success_rate': 0.7,  # Default 70% success
            'keystroke_variance': 0.3,  # Default variance
            'samples': 0
        }
        
    def normalize_error_message(self, error_msg: str) -> str:
        """
        Normalize error message by removing variable parts
        
        Args:
            error_msg: Raw error message
            
        Returns:
            Normalized error pattern
        """
        normalized = error_msg
        
        for pattern, replacement in self.normalization_patterns:
            normalized = re.sub(pattern, replacement, normalized)
        
        # Remove extra whitespace
        normalized = ' '.join(normalized.split())
        
        return normalized
    
    def classify_error(self, error_msg: str) -> str:
        """
        Classify error type based on message content
        
        Args:
            error_msg: Error message
            
        Returns:
            Error category
        """
        error_msg_lower = error_msg.lower()
        
        for error_type, pattern in self.error_patterns.items():
            if pattern.search(error_msg_lower):
                return error_type
        
        return 'unknown_error'
    
    def extract_typing_features(self, keystrokes: List[Dict]) -> Dict[str, float]:
        """
        Extract features related to typing behavior
        
        Args:
            keystrokes: List of keystroke events
            
        Returns:
            Dictionary of typing features
        """
        if len(keystrokes) < 10:
            return {
                'typing_speed': 1.0,
                'keystroke_variance': 0.5,
                'backspace_rate': 0.0,
                'burst_speed': 0.0,
                'pause_frequency': 0.0
            }
        
        # Convert timestamps to seconds
        timestamps = [ks['timestamp'] for ks in keystrokes]
        if isinstance(timestamps[0], str):
            timestamps = [datetime.fromisoformat(ts) for ts in timestamps]
        
        # Calculate intervals between keystrokes (in milliseconds)
        intervals = []
        for i in range(1, len(timestamps)):
            interval = (timestamps[i] - timestamps[i-1]).total_seconds() * 1000
            if 10 < interval < 5000:  # Ignore very short/long intervals
                intervals.append(interval)
        
        if not intervals:
            return {
                'typing_speed': 1.0,
                'keystroke_variance': 0.5,
                'backspace_rate': 0.0,
                'burst_speed': 0.0,
                'pause_frequency': 0.0
            }
        
        # Typing speed (characters per second)
        total_time = (timestamps[-1] - timestamps[0]).total_seconds()
        if total_time > 0:
            chars_per_sec = len(keystrokes) / total_time
            wpm = chars_per_sec * 12  # Convert to WPM (assuming 5 chars/word)
        else:
            wpm = 0
        
        # Keystroke variance (coefficient of variation)
        mean_interval = float(np.mean(intervals))
        std_interval = float(np.std(intervals))
        variance = std_interval / mean_interval if mean_interval > 0 else 0.5
        
        # Backspace rate
        backspaces = sum(1 for ks in keystrokes if ks.get('is_backspace', False))
        backspace_rate = backspaces / len(keystrokes) if keystrokes else 0
        
        # Burst speed (fastest 10% of intervals)
        sorted_intervals = sorted(intervals)
        burst_threshold = int(len(sorted_intervals) * 0.1)
        burst_intervals = sorted_intervals[:burst_threshold] if burst_threshold > 0 else [mean_interval]
        burst_speed = 1000 / float(np.mean(burst_intervals)) if burst_intervals else 0  # chars per second
        
        # Pause frequency (intervals > 2 seconds)
        pauses = sum(1 for i in intervals if i > 2000)
        pause_freq = pauses / len(intervals) if intervals else 0
        
        return {
            'typing_speed': float(wpm / self.baseline['typing_speed'] if self.baseline['typing_speed'] > 0 else 1.0),
            'keystroke_variance': float(variance),
            'backspace_rate': float(backspace_rate),
            'burst_speed': float(burst_speed / 10.0),  # Normalize
            'pause_frequency': float(pause_freq)
        }
    
    def extract_compile_features(self, compiles: List[Dict]) -> Dict[str, float]:
        """
        Extract features related to compilation behavior
        
        Args:
            compiles: List of compile events
            
        Returns:
            Dictionary of compilation features
        """
        if not compiles:
            return {
                'compile_error_rate': 0.0,
                'red_metric': 0.0,
                'warning_rate': 0.0,
                'error_severity': 0.0,
                'repeated_error_ratio': 0.0
            }
        
        # Error rate
        total = len(compiles)
        failed = sum(1 for c in compiles if not c.get('success', True))
        error_rate = failed / total if total > 0 else 0
        
        # Warning rate
        total_warnings = sum(c.get('warning_count', 0) for c in compiles)
        warning_rate = total_warnings / total if total > 0 else 0
        
        # RED Metric (Repeated Error Density)
        error_patterns = []
        for compile_event in compiles:
            if not compile_event.get('success', True) and 'error_message' in compile_event:
                normalized = self.normalize_error_message(compile_event['error_message'])
                error_patterns.append(normalized)
        
        # Calculate repeats
        if len(error_patterns) > 1:
            repeats = 0
            for i in range(1, len(error_patterns)):
                if error_patterns[i] == error_patterns[i-1]:
                    repeats += 1
            red_metric = repeats / len(error_patterns) * 10  # Scale to 0-10
        else:
            red_metric = 0
        
        # Error severity (based on type)
        severity_map = {
            'segmentation_fault': 1.0,
            'memory_leak': 0.9,
            'null_pointer': 0.8,
            'buffer_overflow': 0.8,
            'undefined_reference': 0.6,
            'syntax_error': 0.5,
            'type_mismatch': 0.4,
            'undeclared': 0.3,
            'warning': 0.1
        }
        
        severities = []
        for compile_event in compiles:
            if not compile_event.get('success', True) and 'error_message' in compile_event:
                error_type = self.classify_error(compile_event['error_message'])
                severities.append(severity_map.get(error_type, 0.5))
        
        error_severity = float(np.mean(severities)) if severities else 0
        
        return {
            'compile_error_rate': float(error_rate),
            'red_metric': float(red_metric),
            'warning_rate': float(warning_rate),
            'error_severity': float(error_severity),
            'repeated_error_ratio': float(red_metric / 10.0)
        }
    
    def extract_behavioral_features(self, session_data: Dict) -> Dict[str, float]:
        """
        Extract higher-level behavioral features
        
        Args:
            session_data: Complete session data
            
        Returns:
            Dictionary of behavioral features
        """
        keystrokes = session_data.get('keystrokes', [])
        compiles = session_data.get('compiles', [])
        
        if len(keystrokes) < 5:
            return {
                'focus_switches': 0.0,
                'idle_ratio': 0.0,
                'session_duration': 0.0,
                'activity_intensity': 0.0,
                'recovery_time': 0.0
            }
        
        # Convert timestamps
        timestamps = [ks['timestamp'] for ks in keystrokes]
        if isinstance(timestamps[0], str):
            timestamps = [datetime.fromisoformat(ts) for ts in timestamps]
        
        session_start = timestamps[0]
        session_end = timestamps[-1]
        duration = (session_end - session_start).total_seconds() / 60.0  # minutes
        
        # Focus switches (gaps > 30 seconds)
        focus_switches = 0
        idle_time = 0
        last_time = timestamps[0]
        
        for ts in timestamps[1:]:
            gap = (ts - last_time).total_seconds()
            if gap > 30:
                focus_switches += 1
                idle_time += gap
            last_time = ts
        
        idle_ratio = idle_time / (duration * 60) if duration > 0 else 0
        
        # Activity intensity (keystrokes per minute)
        intensity = len(keystrokes) / duration if duration > 0 else 0
        
        # Recovery time after errors (time to next successful compile)
        recovery_times = []
        for i, compile_event in enumerate(compiles[:-1]):
            if not compile_event.get('success', True):
                # Find next successful compile
                for j in range(i + 1, len(compiles)):
                    if compiles[j].get('success', False):
                        # Time between error and success
                        error_time = compile_event['timestamp']
                        success_time = compiles[j]['timestamp']
                        if isinstance(error_time, str):
                            error_time = datetime.fromisoformat(error_time)
                            success_time = datetime.fromisoformat(success_time)
                        
                        recovery = (success_time - error_time).total_seconds()
                        if recovery < 300:  # Only count recoveries under 5 minutes
                            recovery_times.append(recovery)
                        break
        
        avg_recovery = float(np.mean(recovery_times)) if recovery_times else 0
        
        return {
            'focus_switches': float(focus_switches),
            'idle_ratio': float(idle_ratio),
            'session_duration': float(duration),
            'activity_intensity': float(intensity / 100.0),  # Normalize
            'recovery_time': float(avg_recovery / 60.0)  # Convert to minutes
        }
    
    def extract_all_features(self, session_data: Dict) -> Dict[str, float]:
        """
        Extract all features from session data
        
        Args:
            session_data: Complete session data from plugin
            
        Returns:
            Dictionary of all features in format expected by model
        """
        keystrokes = session_data.get('keystrokes', [])
        compiles = session_data.get('compiles', [])
        
        # Update rolling windows
        for ks in keystrokes[-self.window_size:]:
            self.rolling_keystrokes.append(ks)
        
        for comp in compiles[-10:]:  # Keep last 10 compiles
            self.rolling_compiles.append(comp)
        
        # Extract different feature groups
        typing_features = self.extract_typing_features(keystrokes)
        compile_features = self.extract_compile_features(compiles)
        behavioral_features = self.extract_behavioral_features(session_data)
        
        # Combine into final feature set (order must match training data)
        features = {
            # 1. Typing Speed (normalized)
            'TYPING_SPEED': float(typing_features['typing_speed']),
            
            # 2. Keystroke Rate (variance)
            'KEYSTROKE_RATE': float(typing_features['keystroke_variance']),
            
            # 3. Backspace Rate
            'BACKSPACE_RATE': float(typing_features['backspace_rate']),
            
            # 4. Compile Error Rate
            'COMPILE_ERROR': float(compile_features['compile_error_rate']),
            
            # 5. RED Metric
            'RED_METRIC': float(compile_features['red_metric']),
            
            # 6. Focus Switches
            'FOCUS_SWITCHES': float(behavioral_features['focus_switches']),
            
            # 7. Idle to Active Ratio
            'IDLE_TO_ACTIVE_RATIO': float(behavioral_features['idle_ratio']),
            
            # 8. Undo/Redo Attempts (proxied by backspace bursts)
            'UNDO_REDO_ATTEMPT': 1.0 if typing_features['backspace_rate'] > 0.4 else 0.0
        }
        
        return features
    
    def update_baseline(self, features: Dict[str, float], weight: float = 0.1):
        """
        Update baseline statistics with new features
        
        Args:
            features: New feature values
            weight: Learning rate for baseline update
        """
        self.baseline['typing_speed'] = (1 - weight) * self.baseline['typing_speed'] + \
                                        weight * features.get('TYPING_SPEED', 1.0) * 40
        self.baseline['backspace_rate'] = (1 - weight) * self.baseline['backspace_rate'] + \
                                          weight * features.get('BACKSPACE_RATE', 0.15)
        self.baseline['keystroke_variance'] = (1 - weight) * self.baseline['keystroke_variance'] + \
                                              weight * features.get('KEYSTROKE_RATE', 0.3)
        self.baseline['samples'] += 1
    
    def get_feature_names(self) -> List[str]:
        """Get list of feature names in order expected by model"""
        return [
            'TYPING_SPEED',
            'KEYSTROKE_RATE',
            'BACKSPACE_RATE',
            'COMPILE_ERROR',
            'RED_METRIC',
            'FOCUS_SWITCHES',
            'IDLE_TO_ACTIVE_RATIO',
            'UNDO_REDO_ATTEMPT'
        ]
    
    def feature_vector_to_dict(self, vector: List[float]) -> Dict[str, float]:
        """Convert feature vector to dictionary"""
        names = self.get_feature_names()
        return {names[i]: float(vector[i]) for i in range(len(vector))}
    
    def dict_to_feature_vector(self, feature_dict: Dict[str, float]) -> List[float]:
        """Convert feature dictionary to vector in correct order"""
        return [float(feature_dict.get(name, 0)) for name in self.get_feature_names()]

# Helper function for JSON serialization
def datetime_parser(dct):
    """Parse datetime strings in JSON"""
    for key, value in dct.items():
        if isinstance(value, str):
            try:
                dct[key] = datetime.fromisoformat(value)
            except (ValueError, TypeError):
                pass
    return dct