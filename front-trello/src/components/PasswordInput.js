import React, { useState, useEffect } from 'react';
import { validatePassword, getPasswordStrength } from '../utils/passwordValidator';

const PasswordInput = ({ 
  value, 
  onChange, 
  placeholder = "Enter password", 
  showStrength = true,
  showValidation = true,
  className = "",
  required = false,
  name = "password"
}) => {
  const [showPassword, setShowPassword] = useState(false);
  const [validation, setValidation] = useState({ isValid: true, message: '' });
  const [strength, setStrength] = useState({ score: 0, level: 'Very Weak', color: '#ff4444' });
  const [isFocused, setIsFocused] = useState(false);

  useEffect(() => {
    if (value) {
      const validationResult = validatePassword(value);
      const strengthResult = getPasswordStrength(value);
      
      setValidation(validationResult);
      setStrength(strengthResult);
    } else {
      setValidation({ isValid: true, message: '' });
      setStrength({ score: 0, level: 'Very Weak', color: '#ff4444' });
    }
  }, [value]);

  const togglePasswordVisibility = () => {
    setShowPassword(!showPassword);
  };

  const baseInputClasses = `
    w-full px-4 py-2 pr-12 border rounded-lg focus:outline-none focus:ring-2 transition-colors
    ${validation.isValid || !value ? 
      'border-gray-300 focus:ring-blue-500 focus:border-blue-500' : 
      'border-red-500 focus:ring-red-500 focus:border-red-500'
    }
  `;

  return (
    <div className="space-y-2">
      {/* Password Input with Toggle */}
      <div className="relative">
        <input
          type={showPassword ? "text" : "password"}
          value={value}
          onChange={(e) => onChange(e.target.value)}
          onFocus={() => setIsFocused(true)}
          onBlur={() => setIsFocused(false)}
          placeholder={placeholder}
          required={required}
          name={name}
          className={`${baseInputClasses} ${className}`}
        />
        
        {/* Eye Icon Toggle */}
        <button
          type="button"
          className="absolute right-3 top-1/2 transform -translate-y-1/2 text-gray-500 hover:text-gray-700 focus:outline-none"
          onClick={togglePasswordVisibility}
          tabIndex={-1}
        >
          {showPassword ? (
            <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13.875 18.825A10.05 10.05 0 0112 19c-4.478 0-8.268-2.943-9.543-7a9.97 9.97 0 011.563-3.029m5.858.908a3 3 0 114.243 4.243M9.878 9.878l4.242 4.242M9.878 9.878L12 12m-2.122-2.122L9.878 9.878m4.242 4.242L12 12m2.121-2.122l2.122-2.122M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
            </svg>
          ) : (
            <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M2.458 12C3.732 7.943 7.523 5 12 5c4.478 0 8.268 2.943 9.542 7-1.274 4.057-5.064 7-9.542 7-4.477 0-8.268-2.943-9.542-7z" />
            </svg>
          )}
        </button>
      </div>

      {/* Password Strength Indicator */}
      {showStrength && value && (isFocused || !validation.isValid) && (
        <div className="space-y-1">
          <div className="flex justify-between items-center text-sm">
            <span className="text-gray-600">Password Strength:</span>
            <span className="font-medium" style={{ color: strength.color }}>
              {strength.level}
            </span>
          </div>
          <div className="w-full bg-gray-200 rounded-full h-2">
            <div 
              className="h-2 rounded-full transition-all duration-300"
              style={{ 
                width: `${strength.score}%`, 
                backgroundColor: strength.color 
              }}
            />
          </div>
        </div>
      )}

      {/* Validation Message */}
      {showValidation && !validation.isValid && value && (
        <div className="flex items-start space-x-2 text-red-600 text-sm">
          <svg className="w-4 h-4 mt-0.5 flex-shrink-0" fill="currentColor" viewBox="0 0 20 20">
            <path fillRule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-7 4a1 1 0 11-2 0 1 1 0 012 0zm-1-9a1 1 0 00-1 1v4a1 1 0 102 0V6a1 1 0 00-1-1z" clipRule="evenodd" />
          </svg>
          <span>{validation.message}</span>
        </div>
      )}

      {/* Password Tips (shown when focused and empty) */}
      {isFocused && !value && (
        <div className="text-sm text-gray-500 bg-gray-50 p-3 rounded-lg">
          <p className="font-medium mb-2">Password Requirements:</p>
          <ul className="space-y-1 text-xs">
            <li>• At least 8 characters long</li>
            <li>• Avoid common passwords</li>
            <li>• Mix uppercase, lowercase, numbers, and symbols</li>
          </ul>
        </div>
      )}
    </div>
  );
};

export default PasswordInput;
