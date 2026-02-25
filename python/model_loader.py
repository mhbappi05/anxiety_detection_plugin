"""
Model Loader Module for Anxiety Detection
Handles loading and inference of trained ML models
"""

import pickle
import numpy as np
import joblib
from typing import Dict, List, Tuple, Optional, Any
import os
import json
from datetime import datetime
from sklearn.preprocessing import LabelEncoder, StandardScaler
from sklearn.svm import SVC

class ModelLoader:
    """Loads and manages the trained anxiety detection model"""
    
    def __init__(self, model_dir: str):
        """
        Initialize model loader
        
        Args:
            model_dir: Directory containing model files
        """
        self.model_dir = model_dir
        self.model = None
        self.label_encoder = None
        self.scaler = None
        self.feature_names = None
        self.model_metadata = {}
        
        # Anxiety level mapping
        self.anxiety_levels = ['Low', 'Moderate', 'High', 'Extreme']
        self.level_colors = {
            'Low': '#00FF00',
            'Moderate': '#FFFF00',
            'High': '#FFA500',
            'Extreme': '#FF0000'
        }
        
    def load_models(self) -> bool:
        """
        Load all model files from directory
        
        Returns:
            True if successful, False otherwise
        """
        try:
            # Load main model
            model_path = os.path.join(self.model_dir, 'best_anxiety_model.pkl')
            if os.path.exists(model_path):
                with open(model_path, 'rb') as f:
                    self.model = pickle.load(f)
                print(f"Loaded model from {model_path}")
            else:
                print(f"Model file not found: {model_path}")
                return False
            
            # Load label encoder
            encoder_path = os.path.join(self.model_dir, 'label_encoder.pkl')
            if os.path.exists(encoder_path):
                with open(encoder_path, 'rb') as f:
                    self.label_encoder = pickle.load(f)
                print(f"Loaded label encoder from {encoder_path}")
            else:
                print(f"Label encoder not found: {encoder_path}")
                return False
            
            # Load scaler
            scaler_path = os.path.join(self.model_dir, 'scaler.pkl')
            if os.path.exists(scaler_path):
                with open(scaler_path, 'rb') as f:
                    self.scaler = pickle.load(f)
                print(f"Loaded scaler from {scaler_path}")
            else:
                print(f"Scaler not found: {scaler_path}")
                return False
            
            # Try to load feature names if available
            features_path = os.path.join(self.model_dir, 'feature_names.json')
            if os.path.exists(features_path):
                with open(features_path, 'r') as f:
                    self.feature_names = json.load(f)
            
            # Load model metadata
            metadata_path = os.path.join(self.model_dir, 'model_metadata.json')
            if os.path.exists(metadata_path):
                with open(metadata_path, 'r') as f:
                    self.model_metadata = json.load(f)
            
            # Verify model compatibility
            if not self.verify_model():
                print("Model verification failed")
                return False
            
            print("All models loaded successfully")
            return True
            
        except Exception as e:
            print(f"Error loading models: {e}")
            return False
    
    def verify_model(self) -> bool:
        """Verify that loaded models are compatible"""
        try:
            # Check if model has expected attributes
            if self.model is None or not hasattr(self.model, 'predict'):
                print("Model doesn't have predict method")
                return False
            
            if not hasattr(self.model, 'predict_proba'):
                print("Model doesn't have predict_proba method")
                return False
            
            # Check label encoder
            if self.label_encoder is None or not hasattr(self.label_encoder, 'classes_'):
                print("Label encoder doesn't have classes_")
                return False
            
            # Check scaler
            if self.scaler is None or not hasattr(self.scaler, 'mean_'):
                print("Scaler doesn't have mean_")
                return False
            
            # Verify feature count
            if hasattr(self.scaler, 'n_features_in_'):
                expected_features = self.scaler.n_features_in_
                if self.feature_names and len(self.feature_names) != expected_features:
                    print(f"Feature count mismatch: {len(self.feature_names)} vs {expected_features}")
            
            return True
            
        except Exception as e:
            print(f"Model verification failed: {e}")
            return False
    
    def predict(self, features: np.ndarray) -> Tuple[str, float, np.ndarray]:
        """
        Make prediction from features
        
        Args:
            features: Feature array (n_samples, n_features)
            
        Returns:
            Tuple of (predicted_class, confidence, probabilities)
        """
        if self.model is None:
            raise ValueError("Model not loaded")
        
        # Scale features
        if self.scaler is not None:
            features_scaled = self.scaler.transform(features)
        else:
            features_scaled = features
        
        # Predict class
        prediction = self.model.predict(features_scaled)
        
        # Get probabilities
        if hasattr(self.model, 'predict_proba'):
            probabilities = self.model.predict_proba(features_scaled)[0]
        else:
            # Fallback for models without probability
            probabilities = np.zeros(len(self.label_encoder.classes_)) if self.label_encoder else np.zeros(4)
            probabilities[prediction[0]] = 1.0
        
        # Decode label
        if self.label_encoder is not None:
            predicted_class = self.label_encoder.inverse_transform(prediction)[0]
        else:
            predicted_class = str(prediction[0])
        
        # Get confidence
        confidence = float(np.max(probabilities))
        
        return predicted_class, confidence, probabilities
    
    def predict_from_dict(self, feature_dict: Dict[str, float]) -> Tuple[str, float, Dict[str, float]]:
        """
        Make prediction from feature dictionary
        
        Args:
            feature_dict: Dictionary of feature values
            
        Returns:
            Tuple of (predicted_class, confidence, class_probabilities)
        """
        # Convert dict to array in correct order
        if self.feature_names:
            feature_vector = [feature_dict.get(name, 0) for name in self.feature_names]
        else:
            # Assume dict is in correct order
            feature_vector = list(feature_dict.values())
        
        features = np.array(feature_vector, dtype=np.float64).reshape(1, -1)
        
        # Predict
        predicted_class, confidence, probabilities = self.predict(features)
        
        # Create probability dict
        if self.label_encoder is not None:
            class_names = self.label_encoder.classes_
        else:
            class_names = [f"Class_{i}" for i in range(len(probabilities))]
        
        prob_dict = {str(class_names[i]): float(probabilities[i]) for i in range(len(probabilities))}
        
        return predicted_class, confidence, prob_dict
    
    def get_anxiety_level(self, confidence: float, probabilities: np.ndarray) -> str:
        """Convert probability to anxiety level string"""
        if self.label_encoder is not None:
            class_idx = np.argmax(probabilities)
            return self.label_encoder.inverse_transform([class_idx])[0]
        else:
            # Fallback mapping
            if confidence < 0.3:
                return 'Low'
            elif confidence < 0.6:
                return 'Moderate'
            elif confidence < 0.8:
                return 'High'
            else:
                return 'Extreme'
    
    def get_feature_importance(self) -> Optional[Dict[str, float]]:
        """
        Get feature importance if model supports it
        
        Returns:
            Dictionary of feature importance or None
        """
        if self.model is None:
            return None
            
        if hasattr(self.model, 'feature_importances_'):
            importances = self.model.feature_importances_
        elif hasattr(self.model, 'coef_'):
            importances = np.abs(self.model.coef_[0])
        else:
            return None
        
        if self.feature_names and len(self.feature_names) == len(importances):
            return {self.feature_names[i]: float(importances[i]) 
                   for i in range(len(importances))}
        else:
            return {f"Feature_{i}": float(importances[i]) 
                   for i in range(len(importances))}
    
    def get_model_info(self) -> Dict[str, Any]:
        """Get information about the loaded model"""
        info = {
            'model_type': type(self.model).__name__ if self.model else 'None',
            'classes': self.label_encoder.classes_.tolist() if self.label_encoder else [],
            'n_features': self.scaler.n_features_in_ if self.scaler and hasattr(self.scaler, 'n_features_in_') else None,
            'metadata': self.model_metadata
        }
        
        # Add feature importance if available
        importance = self.get_feature_importance()
        if importance:
            info['feature_importance'] = importance
        
        return info
    
    def validate_features(self, feature_dict: Dict[str, float]) -> Tuple[bool, List[str]]:
        """
        Validate that all required features are present
        
        Args:
            feature_dict: Feature dictionary to validate
            
        Returns:
            Tuple of (is_valid, missing_features)
        """
        if not self.feature_names:
            return True, []
        
        missing = [name for name in self.feature_names if name not in feature_dict]
        return len(missing) == 0, missing

# Utility function to save feature names
def save_feature_names(feature_names: List[str], save_path: str):
    """Save feature names to JSON file"""
    with open(save_path, 'w') as f:
        json.dump(feature_names, f, indent=2)

# Utility function to create model metadata
def create_model_metadata(model, accuracy: float, precision: float, recall: float, f1_score: float):
    """Create metadata file for model"""
    metadata = {
        'accuracy': accuracy,
        'precision': precision,
        'recall': recall,
        'f1_score': f1_score,
        'created': datetime.now().isoformat(),
        'model_type': type(model).__name__
    }
    return metadata