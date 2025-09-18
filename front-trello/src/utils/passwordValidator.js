// Frontend password validation utility
// This provides immediate user feedback - backend validation is still enforced

const commonPasswords = [
  '123456', 'password', 'password123', 'admin', 'qwerty', '123456789', 'letmein',
  '1234567890', 'welcome', 'monkey', 'login', 'abc123', 'starwars', '123123',
  'dragon', 'passw0rd', 'master', 'hello', 'freedom', 'whatever', 'qazwsx',
  'trustno1', '654321', 'jordan23', 'harley', 'password1', '1234', 'robert',
  'matthew', 'jordan', 'michelle', 'mindy', 'patrick', '123abc', 'andrew',
  'tigger', 'sunshine', 'loveme', 'fuckyou', 'iloveyou', 'shadow', '1234567',
  'princess', 'azerty', 'trustno1', '000000', 'secret', 'summer', 'michael',
  'superman', 'batman', 'test', 'pass', 'killer', 'hockey', 'george',
  'charlie', 'andrew', 'michelle', 'love', 'sunshine', 'ashley', 'bailey',
  'passw0rd', 'shadow', '123qwe', '654321', 'admin123', 'root', 'toor',
  'pass123', 'temp', 'guest', 'info', 'adm', 'administrator', 'sa', 'oracle',
  'web', 'demo', '1111', '2000', '2222', '2001', '12341234', '123321',
  '1234qwer', 'qwer1234', 'qwertyuiop', '1qaz2wsx', 'zaq12wsx', 'xsw21qaz',
  '123654', '159753', '147258', '147852', '159357', 'qweqwe', 'asdasd',
  'zxczxc', 'qweasd', 'asdfgh', 'zxcvbn', 'qwerty123', 'asdf1234', 'zxcv1234',
  '1q2w3e4r', '1qaz2wsx3edc', 'a1b2c3d4', 'abcd1234', 'test123', 'user',
  'password12', 'welcome123', 'changeme', 'newpassword'
];

/**
 * Password validation rules
 */
export const PASSWORD_RULES = {
  MIN_LENGTH: 8,
  MAX_LENGTH: 100
};

/**
 * Password validation result codes
 */
export const PASSWORD_ERRORS = {
  VALID: 'VALID',
  TOO_SHORT: 'TOO_SHORT',
  TOO_COMMON: 'TOO_COMMON',
  INVALID_CHARS: 'INVALID_CHARS'
};

/**
 * Validate a password against security rules
 * @param {string} password - The password to validate
 * @returns {object} Validation result with isValid, error, and message
 */
export const validatePassword = (password) => {
  if (!password || typeof password !== 'string') {
    return {
      isValid: false,
      error: PASSWORD_ERRORS.INVALID_CHARS,
      message: 'Password is required'
    };
  }

  // Check minimum length
  if (password.length < PASSWORD_RULES.MIN_LENGTH) {
    return {
      isValid: false,
      error: PASSWORD_ERRORS.TOO_SHORT,
      message: `Password must be at least ${PASSWORD_RULES.MIN_LENGTH} characters long`
    };
  }

  // Check maximum length
  if (password.length > PASSWORD_RULES.MAX_LENGTH) {
    return {
      isValid: false,
      error: PASSWORD_ERRORS.INVALID_CHARS,
      message: `Password must be less than ${PASSWORD_RULES.MAX_LENGTH} characters`
    };
  }

  // Check for invalid characters (basic validation)
  const invalidChars = /[^\x20-\x7E]/; // Allow printable ASCII characters
  if (invalidChars.test(password)) {
    return {
      isValid: false,
      error: PASSWORD_ERRORS.INVALID_CHARS,
      message: 'Password contains invalid characters'
    };
  }

  // Check if password is too common
  if (isPasswordCommon(password)) {
    return {
      isValid: false,
      error: PASSWORD_ERRORS.TOO_COMMON,
      message: 'Password is too common. Please choose a stronger password'
    };
  }

  return {
    isValid: true,
    error: PASSWORD_ERRORS.VALID,
    message: 'Password is valid'
  };
};

/**
 * Check if password is in the common passwords list
 * @param {string} password - The password to check
 * @returns {boolean} True if password is common
 */
export const isPasswordCommon = (password) => {
  if (!password) return false;
  
  // Case-insensitive comparison
  const lowercasePassword = password.toLowerCase();
  return commonPasswords.some(commonPwd => 
    commonPwd.toLowerCase() === lowercasePassword
  );
};

/**
 * Get password strength score (0-100)
 * @param {string} password - The password to score
 * @returns {object} Strength score and level
 */
export const getPasswordStrength = (password) => {
  if (!password) {
    return { score: 0, level: 'Very Weak', color: '#ff4444' };
  }

  let score = 0;

  // Length scoring
  if (password.length >= 8) score += 25;
  if (password.length >= 12) score += 25;

  // Character variety
  if (/[a-z]/.test(password)) score += 10;
  if (/[A-Z]/.test(password)) score += 10;
  if (/[0-9]/.test(password)) score += 10;
  if (/[^A-Za-z0-9]/.test(password)) score += 15;

  // Penalty for common patterns
  if (/(.)\1{2,}/.test(password)) score -= 10; // Repeated characters
  if (/123|abc|qwe/i.test(password)) score -= 15; // Sequential patterns

  // Penalty for common passwords
  if (isPasswordCommon(password)) score -= 50;

  score = Math.max(0, Math.min(100, score));

  let level, color;
  if (score < 30) {
    level = 'Very Weak';
    color = '#ff4444';
  } else if (score < 50) {
    level = 'Weak';
    color = '#ff8800';
  } else if (score < 70) {
    level = 'Fair';
    color = '#ffcc00';
  } else if (score < 90) {
    level = 'Good';
    color = '#88cc00';
  } else {
    level = 'Strong';
    color = '#44cc44';
  }

  return { score, level, color };
};

/**
 * Generate password suggestions
 * @returns {array} Array of password suggestions
 */
export const getPasswordSuggestions = () => {
  return [
    'Use at least 8 characters',
    'Include uppercase and lowercase letters',
    'Add numbers and special characters',
    'Avoid common words and patterns',
    'Don\'t use personal information',
    'Consider using a passphrase with spaces'
  ];
};

export default {
  validatePassword,
  isPasswordCommon,
  getPasswordStrength,
  getPasswordSuggestions,
  PASSWORD_RULES,
  PASSWORD_ERRORS
};
