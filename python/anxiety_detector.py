#!/usr/bin/env python
"""
Anxiety Detection Service for Code::Blocks
Main entry point for the Python backend
"""

import sys
import json
import os
import time
import threading
from datetime import datetime
import win32pipe
import win32file
import pywintypes
import win32security
import win32con

from feature_extractor import FeatureExtractor, datetime_parser
from model_loader import ModelLoader

class AnxietyDetector:
    """Main anxiety detection class"""
    
    def __init__(self, model_dir: str):
        """
        Initialize detector
        
        Args:
            model_dir: Directory containing model files
        """
        self.model_dir = model_dir
        self.model_loader = ModelLoader(model_dir)
        self.feature_extractor = FeatureExtractor()
        
        # Load models
        if not self.model_loader.load_models():
            raise RuntimeError("Failed to load models")
        
        # Session tracking
        self.current_session = {}
        self.last_intervention = None
        self.intervention_cooldown = 300  # 5 minutes
        
        # Get feature importance for explanation
        self.feature_importance = self.model_loader.get_feature_importance()
        
        print(f"Anxiety Detector initialized with models from {model_dir}")
        model_info = self.model_loader.get_model_info()
        print(f"Model type: {model_info['model_type']}")
        print(f"Classes: {model_info['classes']}")
    
    def analyze_session(self, session_data: dict) -> dict:
        """
        Analyze a programming session
        
        Args:
            session_data: Raw session data from plugin
            
        Returns:
            Analysis results with prediction and metrics
        """
        # Parse datetime strings
        if 'session_start' in session_data and isinstance(session_data['session_start'], str):
            session_data['session_start'] = datetime.fromisoformat(session_data['session_start'])
        if 'last_activity' in session_data and isinstance(session_data['last_activity'], str):
            session_data['last_activity'] = datetime.fromisoformat(session_data['last_activity'])
        
        if 'keystrokes' in session_data:
            for ks in session_data['keystrokes']:
                if 'timestamp' in ks and isinstance(ks['timestamp'], str):
                    ks['timestamp'] = datetime.fromisoformat(ks['timestamp'])
        
        if 'compiles' in session_data:
            for comp in session_data['compiles']:
                if 'timestamp' in comp and isinstance(comp['timestamp'], str):
                    comp['timestamp'] = datetime.fromisoformat(comp['timestamp'])
        
        # Extract features
        features = self.feature_extractor.extract_all_features(session_data)
        
        # Update baseline
        self.feature_extractor.update_baseline(features)
        
        # Make prediction
        predicted_class, confidence, probabilities = self.model_loader.predict_from_dict(features)
        
        # Determine triggered features
        triggered = self.get_triggered_features(features)
        
        # Check if intervention is needed
        should_intervene = self.should_intervene(predicted_class, confidence)
        
        return {
            'level': predicted_class,
            'confidence': confidence,
            'probabilities': probabilities,
            'features': features,
            'triggered_features': triggered,
            'timestamp': datetime.now().isoformat(),
            'should_intervene': should_intervene
        }
    
    def get_triggered_features(self, features: dict) -> list:
        """Determine which features triggered high anxiety"""
        triggered = []
        
        # Thresholds from research
        thresholds = {
            'RED_METRIC': 2.5,
            'TYPING_SPEED': 0.65,  # 35% drop from baseline
            'BACKSPACE_RATE': 0.3,
            'KEYSTROKE_RATE': 0.5
        }
        
        if features.get('RED_METRIC', 0) > thresholds['RED_METRIC']:
            triggered.append('Repeated Errors')
        
        if features.get('TYPING_SPEED', 1) < thresholds['TYPING_SPEED']:
            triggered.append('Slow Typing')
        
        if features.get('BACKSPACE_RATE', 0) > thresholds['BACKSPACE_RATE']:
            triggered.append('Excessive Corrections')
        
        if features.get('KEYSTROKE_RATE', 0) > thresholds['KEYSTROKE_RATE']:
            triggered.append('Irregular Rhythm')
        
        if features.get('COMPILE_ERROR', 0) > 0.5:
            triggered.append('Frequent Compilation Errors')
        
        if features.get('FOCUS_SWITCHES', 0) > 5:
            triggered.append('Frequent Context Switching')
        
        return triggered if triggered else ['Normal Pattern']
    
    def should_intervene(self, anxiety_level: str, confidence: float) -> bool:
        """Determine if intervention should be triggered"""
        # Check cooldown
        if self.last_intervention:
            cooldown_elapsed = (datetime.now() - self.last_intervention).total_seconds()
            if cooldown_elapsed < self.intervention_cooldown:
                return False
        
        # Check anxiety level
        if anxiety_level in ['High', 'Extreme'] and confidence > 0.7:
            self.last_intervention = datetime.now()
            return True
        
        return False
    
    def get_error_hint(self, error_type: str) -> str:
        """Get helpful hint for error type"""
        hints = {
            'syntax_error': "Check for missing semicolons, brackets, or parentheses",
            'missing_semicolon': "You might be missing a semicolon at the end of a statement",
            'undefined_reference': "You might be missing a header file or library link",
            'missing_header': "Check if you've included the necessary header files",
            'segmentation_fault': "Check for null pointers or array bounds",
            'null_pointer': "Make sure to initialize pointers before using them",
            'array_bounds': "Ensure array indices are within bounds",
            'uninitialized': "Initialize variables before using them",
            'memory_leak': "Remember to free allocated memory",
            'buffer_overflow': "Check array sizes and string lengths",
            'type_mismatch': "Ensure types are compatible",
            'no_matching_function': "Check function parameters and overloads",
            'ambiguous': "Make the call more specific",
            'redefinition': "Remove duplicate declarations",
            'undeclared': "Declare variables before using them",
            'incomplete_type': "Include the full type definition"
        }
        
        for key, hint in hints.items():
            if key in error_type.lower():
                return hint
        
        return "Take a deep breath. Try breaking down the problem into smaller parts."

class PipeServer:
    """Named pipe server for communication with Code::Blocks"""
    
    def __init__(self, pipe_name=r'\\.\pipe\AnxietyDetector'):
        self.pipe_name = pipe_name
        self.detector = None
        self.running = True
        self.pipe = None
    
    def initialize_detector(self, model_dir):
        """Initialize the anxiety detector"""
        try:
            self.detector = AnxietyDetector(model_dir)
            return True
        except Exception as e:
            print(f"Failed to initialize detector: {e}")
            return False
    
    def run(self):
        """Run the pipe server"""
        print(f"Starting pipe server on {self.pipe_name}")
        
        while self.running:
            try:
                # Create a simple security descriptor that allows all access
                # This avoids the None parameter issue
                security_attributes = win32security.SECURITY_ATTRIBUTES()
                security_attributes.bInheritHandle = 1
                
                # Create named pipe with proper security attributes
                self.pipe = win32pipe.CreateNamedPipe(
                    self.pipe_name,
                    win32pipe.PIPE_ACCESS_DUPLEX,
                    win32pipe.PIPE_TYPE_MESSAGE | win32pipe.PIPE_READMODE_MESSAGE | win32pipe.PIPE_WAIT,
                    win32pipe.PIPE_UNLIMITED_INSTANCES,
                    65536, 65536, 0,
                    security_attributes  # Use security attributes object, not None
                )
                
                if self.pipe == win32file.INVALID_HANDLE_VALUE:
                    print("Failed to create named pipe")
                    time.sleep(1)
                    continue
                
                print("Waiting for client connection...")
                # Wait for client to connect
                result = win32pipe.ConnectNamedPipe(self.pipe, None)
                print(f"Client connected (result: {result})")
                
                # Handle client requests
                while self.running:
                    try:
                        # Read request
                        hr, data = win32file.ReadFile(self.pipe, 65536)
                        if hr != 0:
                            print(f"Read error: {hr}")
                            break
                            
                        # Handle data based on Python version
                        if isinstance(data, bytes):
                            json_str = data.decode('utf-8')
                        else:
                            json_str = data
                            
                        request_data = json.loads(json_str)
                        
                        # Process request
                        response = self.handle_request(request_data)
                        
                        # Send response
                        json_response = json.dumps(response, default=str)
                        win32file.WriteFile(self.pipe, json_response.encode('utf-8'))
                        
                    except pywintypes.error as e:
                        if e.winerror == 109:  # Broken pipe
                            print("Client disconnected")
                            break
                        elif e.winerror == 232:  # Pipe is being closed
                            print("Pipe closing")
                            break
                        else:
                            print(f"Pipe error: {e}")
                            break
                    except json.JSONDecodeError as e:
                        print(f"JSON decode error: {e}")
                        # Send error response
                        error_response = {'status': 'error', 'message': 'Invalid JSON'}
                        try:
                            win32file.WriteFile(self.pipe, json.dumps(error_response).encode('utf-8'))
                        except:
                            pass
                    except Exception as e:
                        print(f"Error processing request: {e}")
                        try:
                            error_response = {'status': 'error', 'message': str(e)}
                            win32file.WriteFile(self.pipe, json.dumps(error_response).encode('utf-8'))
                        except:
                            pass
                
                # Disconnect pipe
                try:
                    win32pipe.DisconnectNamedPipe(self.pipe)
                    win32file.CloseHandle(self.pipe)
                except:
                    pass
                self.pipe = None
                
            except KeyboardInterrupt:
                print("\nShutting down...")
                self.running = False
                break
            except Exception as e:
                print(f"Server error: {e}")
                if self.pipe:
                    try:
                        win32file.CloseHandle(self.pipe)
                    except:
                        pass
                    self.pipe = None
                time.sleep(1)
    
    def handle_request(self, data):
        """Handle client requests"""
        request_type = data.get('type')
        
        if request_type == 'initialize':
            model_dir = data.get('model_dir')
            success = self.initialize_detector(model_dir)
            return {
                'status': 'ok' if success else 'error',
                'message': 'Detector initialized' if success else 'Initialization failed'
            }
        
        elif request_type == 'analyze':
            if not self.detector:
                return {'status': 'error', 'message': 'Detector not initialized'}
            
            session_data = data.get('session', {})
            result = self.detector.analyze_session(session_data)
            
            return {
                'status': 'ok',
                'prediction': result
            }
        
        elif request_type == 'get_hint':
            if not self.detector:
                return {'status': 'error', 'message': 'Detector not initialized'}
            
            error_type = data.get('error_type', '')
            hint = self.detector.get_error_hint(error_type)
            
            return {
                'status': 'ok',
                'hint': hint
            }
        
        elif request_type == 'shutdown':
            self.running = False
            return {'status': 'ok', 'message': 'Shutting down'}
        
        return {'status': 'error', 'message': f'Unknown request type: {request_type}'}

def main():
    """Main entry point"""
    if len(sys.argv) < 2:
        print("Usage: anxiety_detector.py <model_directory>")
        print("Example: anxiety_detector.py C:\\Program Files\\CodeBlocks\\share\\CodeBlocks\\anxiety_models")
        sys.exit(1)
    
    model_dir = sys.argv[1]
    
    if not os.path.exists(model_dir):
        print(f"Error: Model directory not found: {model_dir}")
        sys.exit(1)
    
    server = PipeServer()
    
    print("=" * 50)
    print("Anxiety Detection Service for Code::Blocks")
    print("=" * 50)
    print(f"Model directory: {model_dir}")
    print("Waiting for Code::Blocks connections...")
    print("Press Ctrl+C to stop")
    print("=" * 50)
    
    try:
        server.run()
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        server.running = False
        if server.pipe:
            try:
                win32file.CloseHandle(server.pipe)
            except:
                pass
        print("Service stopped")

if __name__ == "__main__":
    main()