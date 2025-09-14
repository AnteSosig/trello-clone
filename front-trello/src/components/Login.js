import React, { useState } from 'react';
import Cookies from 'js-cookie';

const LoginForm = () => {
  const [username_or_email, setUsernameOrEmail] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');
  const [successMessage, setSuccessMessage] = useState('');

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
      const expirationInSeconds = data.expires || 3600; // default 1 hour if not provided
      const expirationInDays = expirationInSeconds / (24 * 60 * 60); // Convert seconds to days

      console.log('Setting cookies with expiration (days):', expirationInDays);

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

      console.log('Stored Cookies:');
      console.log('Token:', Cookies.get('token'));
      console.log('Role:', Cookies.get('role'));
      console.log('Session Expiration:', Cookies.get('sessionExpiration'));
      
      // Or log all cookies at once
      console.log('All Cookies:', Cookies.get());

      setSuccessMessage('Uspešno ste ulogovani.');
      
    } catch (error) {
      console.error('Login error:', error);
      setError(error.message);
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
      </div>
    </div>
  );
};

export default LoginForm;
