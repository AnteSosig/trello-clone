import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import Cookies from 'js-cookie';
import { useNavigate, useLocation } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';

const LoginForm = () => {
  const navigate = useNavigate();
  const [username_or_email, setUsernameOrEmail] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');
  const [successMessage, setSuccessMessage] = useState('');
  const [showRecoveryForm, setShowRecoveryForm] = useState(false);
  const [recoveryEmail, setRecoveryEmail] = useState('');
  const [recoveryError, setRecoveryError] = useState('');
  const [recoverySuccess, setRecoverySuccess] = useState('');

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError('');
    setSuccessMessage('');
    
    console.log('Attempting login with:', { username_or_email, password });

    try {
      const response = await fetch('https://localhost:8443/user/login', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ username_or_email, password }),
      });

      console.log('Response status:', response.status);
      
      if (!response.ok) {
        throw new Error('Pogrešni podaci za prijavu.');
      }

      const data = await response.json();
      console.log('Response data:', data);
      
      // Set cookies with expiration
      const expirationInSeconds = data.expires || 3600;
      const expirationInDays = expirationInSeconds / (24 * 60 * 60);

      Cookies.set('token', data.token, { 
        expires: expirationInDays,
        sameSite: 'strict'
      });
      
      Cookies.set('role', data.role, {
        expires: expirationInDays,
        sameSite: 'strict'
      });

      Cookies.set('sessionExpiration', new Date(Date.now() + expirationInSeconds * 1000).toISOString(), {
        expires: expirationInDays,
        sameSite: 'strict'
      });

      // Update auth context
      if (login(data.token, data.role)) {
        setSuccessMessage('Uspešno ste ulogovani.');
        
        // Add a small delay before redirecting to show the success message
        setTimeout(() => {
          navigate(from, { replace: true });
        }, 1000);
      } else {
        setError('Failed to process login data');
      }
      
      // Redirect to home page after successful login and refresh
      setTimeout(() => {
        navigate('/');
        window.location.reload();
      }, 1000);
      
    } catch (error) {
      console.error('Login error:', error);
      setError(error.message);
    }
  };

  const handlePasswordRecovery = async (e) => {
    e.preventDefault();
    setRecoveryError('');
    setRecoverySuccess('');

    if (!recoveryEmail) {
      setRecoveryError('Molimo unesite email adresu.');
      return;
    }

    try {
      const response = await fetch('https://localhost:8443/user/recoverpassword', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ email: recoveryEmail }),
      });

      if (!response.ok) {
        throw new Error('Greška prilikom slanja zahteva za oporavak lozinke.');
      }

      setRecoverySuccess('Zahtev za oporavak lozinke je uspešno poslat. Proverite svoj email.');
      setRecoveryEmail('');
      
      // Hide recovery form after 3 seconds
      setTimeout(() => {
        setShowRecoveryForm(false);
        setRecoverySuccess('');
      }, 3000);

    } catch (error) {
      console.error('Password recovery error:', error);
      setRecoveryError(error.message);
    }
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 py-12 px-4 sm:px-6 lg:px-8">
      <div className="max-w-md mx-auto bg-white/10 backdrop-blur-lg rounded-xl p-8 shadow-2xl">
        <div className="text-center mb-8">
          <h2 className="text-3xl font-bold text-white">Prijava</h2>
        </div>

        {error && (
          <div className="mb-4 p-4 bg-red-500/20 backdrop-blur-lg border border-red-500/50 rounded-lg text-white">
            {error}
          </div>
        )}
        
        {successMessage && (
          <div className="mb-4 p-4 bg-green-500/20 backdrop-blur-lg border border-green-500/50 rounded-lg text-white">
            {successMessage}
          </div>
        )}

        <form onSubmit={handleSubmit} className="space-y-6">
          <div>
            <label htmlFor="username_or_email" className="block text-sm font-medium text-white mb-2">
              Korisničko ime ili email
            </label>
            <input
              type="text"
              id="username_or_email"
              value={username_or_email}
              onChange={(e) => setUsernameOrEmail(e.target.value)}
              className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg 
                       text-white placeholder-white/50 focus:outline-none focus:ring-2 
                       focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg"
              placeholder="Unesite korisničko ime ili email"
              required
            />
          </div>

          <div>
            <label htmlFor="password" className="block text-sm font-medium text-white mb-2">
              Lozinka
            </label>
            <input
              type="password"
              id="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg 
                       text-white placeholder-white/50 focus:outline-none focus:ring-2 
                       focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg"
              placeholder="Unesite lozinku"
              required
            />
          </div>

          <button
            type="submit"
            className="w-full px-4 py-2 bg-emerald-600 text-white rounded-lg 
                     hover:bg-emerald-700 transition-colors duration-200 font-medium"
          >
            Prijavi se
          </button>
        </form>

        {/* Password Recovery Button */}
        <div className="mt-4">
          <button
            type="button"
            onClick={() => setShowRecoveryForm(!showRecoveryForm)}
            className="w-full px-4 py-2 bg-emerald-600 text-white rounded-lg 
                     hover:bg-emerald-700 transition-colors duration-200 font-medium"
          >
            Oporavak lozinke
          </button>
        </div>

        {/* Password Recovery Form */}
        {showRecoveryForm && (
          <div className="mt-6 p-6 bg-white/5 backdrop-blur-lg rounded-lg border border-white/10">
            <h3 className="text-xl font-semibold text-white mb-4 text-center">
              Oporavak lozinke
            </h3>

            {recoveryError && (
              <div className="mb-4 p-4 bg-red-500/20 backdrop-blur-lg border border-red-500/50 rounded-lg text-white">
                {recoveryError}
              </div>
            )}
            
            {recoverySuccess && (
              <div className="mb-4 p-4 bg-green-500/20 backdrop-blur-lg border border-green-500/50 rounded-lg text-white">
                {recoverySuccess}
              </div>
            )}

            <form onSubmit={handlePasswordRecovery} className="space-y-4">
              <div>
                <label htmlFor="recovery_email" className="block text-sm font-medium text-white mb-2">
                  Email adresa
                </label>
                <input
                  type="email"
                  id="recovery_email"
                  value={recoveryEmail}
                  onChange={(e) => setRecoveryEmail(e.target.value)}
                  className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg 
                           text-white placeholder-white/50 focus:outline-none focus:ring-2 
                           focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg"
                  placeholder="Unesite vašu email adresu"
                  required
                />
              </div>

              <div className="flex space-x-3">
                <button
                  type="submit"
                  className="flex-1 px-4 py-2 bg-emerald-600 text-white rounded-lg 
                           hover:bg-emerald-700 transition-colors duration-200 font-medium"
                >
                  Pošalji zahtev
                </button>
                <button
                  type="button"
                  onClick={() => {
                    setShowRecoveryForm(false);
                    setRecoveryEmail('');
                    setRecoveryError('');
                    setRecoverySuccess('');
                  }}
                  className="flex-1 px-4 py-2 bg-gray-600 text-white rounded-lg 
                           hover:bg-gray-700 transition-colors duration-200 font-medium"
                >
                  Otkaži
                </button>
              </div>
            </form>
          </div>
        )}
      </div>
    </div>
  );
};

export default LoginForm;
