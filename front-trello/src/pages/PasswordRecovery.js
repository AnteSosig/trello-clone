import React, { useEffect, useState, useRef } from "react";
import { useNavigate } from "react-router-dom";

const PasswordRecovery = () => {
  const navigate = useNavigate();
  const [status, setStatus] = useState(null); // null for loading, true for valid link, false for error
  const [newPassword, setNewPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');
  const [passwordError, setPasswordError] = useState('');
  const [submitting, setSubmitting] = useState(false);
  const [successMessage, setSuccessMessage] = useState('');
  const linkValidationAttempted = useRef(false);
  const recoveryLink = useRef(null);

  useEffect(() => {
    if (linkValidationAttempted.current) return;
    
    const queryParams = new URLSearchParams(window.location.search);
    const link = queryParams.get("link");
    recoveryLink.current = link;

    if (link) {
      linkValidationAttempted.current = true;
      
      // Validate the recovery link with the backend
      fetch(`https://localhost:8443/user/confirmrecovercode?link=${link}`, {
        method: 'GET',
      })
        .then((response) => {
          if (response.status === 200) {
            setStatus(true); // Valid link
          } else {
            setStatus(false); // Invalid link
          }
        })
        .catch((error) => {
          console.error('Link validation error:', error);
          setStatus(false); // Failed validation
        });
    } else {
      setStatus(false); // No link provided
    }
  }, []);

  const handlePasswordSubmit = async (e) => {
    e.preventDefault();
    setPasswordError('');
    setSuccessMessage('');

    // Validate passwords match
    if (newPassword !== confirmPassword) {
      setPasswordError('Lozinke se ne poklapaju.');
      return;
    }

    // Validate password length
    if (newPassword.length < 6) {
      setPasswordError('Lozinka mora imati najmanje 6 karaktera.');
      return;
    }

    setSubmitting(true);

    try {
      const response = await fetch(`https://localhost:8443/user/confirmpasswordrecovery?link=${recoveryLink.current}`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ new_password: newPassword }),
      });

      if (response.status === 200) {
        setSuccessMessage('Lozinka je uspešno promenjena. Možete se sada prijaviti sa novom lozinkom.');
        setNewPassword('');
        setConfirmPassword('');
        
        // Redirect to login after 3 seconds
        setTimeout(() => {
          navigate('/login');
        }, 3000);
      } else {
        setPasswordError('Greška prilikom promene lozinke. Molimo pokušajte ponovo.');
      }
    } catch (error) {
      console.error('Password change error:', error);
      setPasswordError('Greška prilikom promene lozinke. Molimo pokušajte ponovo.');
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <div className="flex items-center justify-center min-h-screen bg-gray-100">
      <div className="max-w-md w-full mx-4">
        {status === null ? (
          <div className="text-center">
            <p className="text-lg text-gray-600">Validacija linka za oporavak...</p>
          </div>
        ) : status === true ? (
          <div className="bg-white shadow-lg rounded-lg p-8">
            <div className="text-center mb-6">
              <h2 className="text-2xl font-bold text-gray-800">Promena lozinke</h2>
              <p className="text-gray-600 mt-2">Unesite novu lozinku</p>
            </div>

            {passwordError && (
              <div className="mb-4 p-4 bg-red-100 border border-red-400 rounded-lg text-red-700">
                {passwordError}
              </div>
            )}

            {successMessage && (
              <div className="mb-4 p-4 bg-green-100 border border-green-400 rounded-lg text-green-700">
                {successMessage}
              </div>
            )}

            <form onSubmit={handlePasswordSubmit} className="space-y-6">
              <div>
                <label htmlFor="new_password" className="block text-sm font-medium text-gray-700 mb-2">
                  Nova lozinka
                </label>
                <input
                  type="password"
                  id="new_password"
                  value={newPassword}
                  onChange={(e) => setNewPassword(e.target.value)}
                  className="w-full px-4 py-2 border border-gray-300 rounded-lg 
                           focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                  placeholder="Unesite novu lozinku"
                  required
                  disabled={submitting}
                />
              </div>

              <div>
                <label htmlFor="confirm_password" className="block text-sm font-medium text-gray-700 mb-2">
                  Potvrda nove lozinke
                </label>
                <input
                  type="password"
                  id="confirm_password"
                  value={confirmPassword}
                  onChange={(e) => setConfirmPassword(e.target.value)}
                  className="w-full px-4 py-2 border border-gray-300 rounded-lg 
                           focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                  placeholder="Potvrdite novu lozinku"
                  required
                  disabled={submitting}
                />
              </div>

              <button
                type="submit"
                disabled={submitting}
                className="w-full px-4 py-2 bg-blue-600 text-white rounded-lg 
                         hover:bg-blue-700 transition-colors duration-200 font-medium
                         disabled:bg-blue-400 disabled:cursor-not-allowed"
              >
                {submitting ? 'Menjanje lozinke...' : 'Promeni lozinku'}
              </button>
            </form>
          </div>
        ) : (
          <div className="bg-white shadow-lg rounded-lg p-8">
            <div className="text-center">
              <div className="mb-4">
                <svg className="mx-auto h-16 w-16 text-red-500" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-2.5L13.732 4c-.77-.833-1.964-.833-2.732 0L4.082 16.5c-.77.833.192 2.5 1.732 2.5z" />
                </svg>
              </div>
              <h2 className="text-2xl font-bold text-red-600 mb-4">Neispravan link</h2>
              <p className="text-gray-600 mb-6">
                Link za oporavak lozinke je neispravan ili je istekao. 
                Molimo zatražite novi link za oporavak lozinke.
              </p>
              <button
                onClick={() => navigate('/login')}
                className="px-6 py-2 bg-blue-600 text-white rounded-lg 
                         hover:bg-blue-700 transition-colors duration-200 font-medium"
              >
                Nazad na prijavu
              </button>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};

export default PasswordRecovery;
