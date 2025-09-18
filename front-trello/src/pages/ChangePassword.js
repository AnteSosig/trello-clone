import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { getCookie } from '../utils/cookies';

const ChangePassword = () => {
  const navigate = useNavigate();
  const [formData, setFormData] = useState({
    oldPassword: '',
    confirmOldPassword: '',
    newPassword: ''
  });
  const [error, setError] = useState('');
  const [successMessage, setSuccessMessage] = useState('');
  const [isLoading, setIsLoading] = useState(false);

  const handleInputChange = (e) => {
    const { name, value } = e.target;
    setFormData(prev => ({
      ...prev,
      [name]: value
    }));
    if (error) setError('');
  };

  const validateForm = () => {
    if (!formData.oldPassword || !formData.confirmOldPassword || !formData.newPassword) {
      setError('Sva polja su obavezna.');
      return false;
    }

    if (formData.oldPassword !== formData.confirmOldPassword) {
      setError('Stara lozinka i potvrda stare lozinke se ne poklapaju.');
      return false;
    }

    if (formData.oldPassword === formData.newPassword) {
      setError('Nova lozinka mora biti drugačija od stare lozinke.');
      return false;
    }

    if (formData.newPassword.length < 6) {
      setError('Nova lozinka mora imati najmanje 6 karaktera.');
      return false;
    }

    return true;
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError('');
    setSuccessMessage('');

    if (!validateForm()) {
      return;
    }

    setIsLoading(true);

    try {
      const token = getCookie('token');
      if (!token) {
        setError('Niste ulogovani. Molimo prijavite se ponovo.');
        setIsLoading(false);
        return;
      }

      console.log('Sending request to change password with token:', token);
      console.log('Request payload:', {
        old_password: formData.oldPassword,
        new_password: formData.newPassword
      });

      const response = await fetch('https://localhost:8443/user/changepassword', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          Authorization: token,
        },
        body: JSON.stringify({
          old_password: formData.oldPassword,
          new_password: formData.newPassword,
        }),
      });

      console.log('Response status:', response.status);
      console.log('Response headers:', response.headers);

      if (!response.ok) {
        let errorMessage = 'Greška pri promeni lozinke.';
        try {
          const errorData = await response.json();
          errorMessage = errorData.message || errorMessage;
        } catch (jsonError) {
          console.log('JSON parse error:', jsonError);
          try {
            const errorText = await response.text();
            console.log('Response text:', errorText);
            errorMessage = errorText || errorMessage;
          } catch (textError) {
            console.log('Text parse error:', textError);
            errorMessage = response.statusText || errorMessage;
          }
        }
        throw new Error(errorMessage);
      }

      setSuccessMessage('Lozinka je uspešno promenjena.');

      setTimeout(() => {
        navigate('/profile');
      }, 2000);

    } catch (error) {
      console.error('Change password error:', error);
      if (error.name === 'TypeError' && error.message.includes('fetch')) {
        setError('Greška mreže. Proverite da li je server pokrenut i dostupan.');
      } else {
        setError(error.message);
      }
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 py-12 px-4 sm:px-6 lg:px-8">
      <div className="max-w-md mx-auto bg-white/10 backdrop-blur-lg rounded-xl p-8 shadow-2xl border border-white/20">
        <div className="text-center mb-8">
          <h2 className="text-3xl font-bold text-white">Promeni lozinku</h2>
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
            <label htmlFor="oldPassword" className="block text-sm font-medium text-white mb-2">
              Stara lozinka
            </label>
            <input
              type="password"
              id="oldPassword"
              name="oldPassword"
              value={formData.oldPassword}
              onChange={handleInputChange}
              className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg 
                       text-white placeholder-white/50 focus:outline-none focus:ring-2 
                       focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg"
              placeholder="Unesite staru lozinku"
              required
            />
          </div>

          <div>
            <label htmlFor="confirmOldPassword" className="block text-sm font-medium text-white mb-2">
              Potvrda stare lozinke
            </label>
            <input
              type="password"
              id="confirmOldPassword"
              name="confirmOldPassword"
              value={formData.confirmOldPassword}
              onChange={handleInputChange}
              className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg 
                       text-white placeholder-white/50 focus:outline-none focus:ring-2 
                       focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg"
              placeholder="Potvrdite staru lozinku"
              required
            />
          </div>

          <div>
            <label htmlFor="newPassword" className="block text-sm font-medium text-white mb-2">
              Nova lozinka
            </label>
            <input
              type="password"
              id="newPassword"
              name="newPassword"
              value={formData.newPassword}
              onChange={handleInputChange}
              className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg 
                       text-white placeholder-white/50 focus:outline-none focus:ring-2 
                       focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg"
              placeholder="Unesite novu lozinku"
              required
            />
          </div>

          <div className="flex gap-4">
            <button
              type="button"
              onClick={() => navigate('/profile')}
              className="flex-1 px-4 py-2 bg-gray-600 text-white rounded-lg 
                       hover:bg-gray-700 transition-colors duration-200 font-medium"
            >
              Otkaži
            </button>
            <button
              type="submit"
              disabled={isLoading}
              className="flex-1 px-4 py-2 bg-emerald-600 text-white rounded-lg 
                       hover:bg-emerald-700 transition-colors duration-200 font-medium
                       disabled:opacity-50 disabled:cursor-not-allowed"
            >
              {isLoading ? 'Promena...' : 'Promeni lozinku'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
};

export default ChangePassword;
