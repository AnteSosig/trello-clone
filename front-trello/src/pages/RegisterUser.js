import React, { useState } from 'react';
import { userApi } from '../utils/axios';
import PasswordInput from '../components/PasswordInput';
import { validatePassword } from '../utils/passwordValidator';

const Register = () => {
  const [formData, setFormData] = useState({
    username: '',
    email: '',
    password: '',
    first_name: '',
    last_name: '',
    role: 'USER'
  });

  const [error, setError] = useState('');
  const [success, setSuccess] = useState('');
  const [isSubmitting, setIsSubmitting] = useState(false);

  const handleChange = (e) => {
    setFormData({ ...formData, [e.target.name]: e.target.value });
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    
    // Validate password on frontend first
    const passwordValidation = validatePassword(formData.password);
    if (!passwordValidation.isValid) {
      setError(`Password validation failed: ${passwordValidation.message}`);
      setSuccess('');
      return;
    }

    setIsSubmitting(true);
    setError('');
    setSuccess('');

    try {
      const userPayload = {
        username: formData.username,
        first_name: formData.first_name,
        last_name: formData.last_name,
        email: formData.email,
        password: formData.password,
        role: formData.role
      };

      const response = await userApi.post('/newuser', userPayload);

      setSuccess('Uspešno ste se registrovali! Proverite email za aktivaciju.');
      setError('');
      
      // Clear form on success
      setFormData({
        username: '',
        email: '',
        password: '',
        first_name: '',
        last_name: '',
        role: 'USER'
      });
    } catch (error) {
      if (error.response?.data?.message) {
        setError(`Registration failed: ${error.response.data.message}`);
      } else if (error.response?.data?.error) {
        setError(`Error: ${error.response.data.error}`);
      } else {
        setError('Došlo je do greške prilikom registracije. Molimo pokušajte ponovo.');
      }
      setSuccess('');
    } finally {
      setIsSubmitting(false);
    }
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 py-12 px-4 sm:px-6 lg:px-8">
      <div className="max-w-md mx-auto bg-white/10 backdrop-blur-lg rounded-xl p-8 shadow-2xl">
        <div className="text-center mb-8">
          <h2 className="text-3xl font-bold text-white">Registracija</h2>
        </div>

        {error && (
          <div className="mb-4 p-4 bg-red-500/20 backdrop-blur-lg border border-red-500/50 rounded-lg text-white">
            {error}
          </div>
        )}
        
        {success && (
          <div className="mb-4 p-4 bg-green-500/20 backdrop-blur-lg border border-green-500/50 rounded-lg text-white">
            {success}
          </div>
        )}

        <form onSubmit={handleSubmit} className="space-y-6">
          <div>
            <label htmlFor="username" className="block text-sm font-medium text-white mb-2">
              Korisničko ime
            </label>
            <input
              type="text"
              id="username"
              name="username"
              value={formData.username}
              onChange={handleChange}
              className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg 
                       text-white placeholder-white/50 focus:outline-none focus:ring-2 
                       focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg"
              required
            />
          </div>

          <div>
            <label htmlFor="first_name" className="block text-sm font-medium text-white mb-2">
              Ime
            </label>
            <input
              type="text"
              id="first_name"
              name="first_name"
              value={formData.first_name}
              onChange={handleChange}
              className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg 
                       text-white placeholder-white/50 focus:outline-none focus:ring-2 
                       focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg"
              required
            />
          </div>

          <div>
            <label htmlFor="last_name" className="block text-sm font-medium text-white mb-2">
              Prezime
            </label>
            <input
              type="text"
              id="last_name"
              name="last_name"
              value={formData.last_name}
              onChange={handleChange}
              className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg 
                       text-white placeholder-white/50 focus:outline-none focus:ring-2 
                       focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg"
              required
            />
          </div>

          <div>
            <label htmlFor="email" className="block text-sm font-medium text-white mb-2">
              Email
            </label>
            <input
              type="email"
              id="email"
              name="email"
              value={formData.email}
              onChange={handleChange}
              className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg 
                       text-white placeholder-white/50 focus:outline-none focus:ring-2 
                       focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg"
              required
            />
          </div>

          <div>
            <label htmlFor="password" className="block text-sm font-medium text-white mb-2">
              Lozinka
            </label>
            <PasswordInput
              value={formData.password}
              onChange={(value) => setFormData({ ...formData, password: value })}
              placeholder="Unesite lozinku"
              className="bg-white/10 border-white/20 text-white placeholder-white/50 
                        focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg"
              showStrength={true}
              showValidation={true}
              required={true}
              name="password"
            />
          </div>

          <div>
            <label htmlFor="role" className="block text-sm font-medium text-white mb-2">
              Uloga
            </label>
            <select
              id="role"
              name="role"
              value={formData.role}
              onChange={handleChange}
              className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg 
                       text-white focus:outline-none focus:ring-2 
                       focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg
                       [&>option]:text-gray-900 [&>option]:bg-white"
            >
              <option value="USER">Korisnik</option>
              <option value="MANAGER">Menadžer</option>
            </select>
          </div>

          <button
            type="submit"
            disabled={isSubmitting}
            className="w-full px-4 py-2 bg-emerald-600 text-white rounded-lg 
                     hover:bg-emerald-700 disabled:bg-emerald-800 disabled:cursor-not-allowed
                     transition-colors duration-200 font-medium flex items-center justify-center"
          >
            {isSubmitting ? (
              <>
                <svg className="animate-spin -ml-1 mr-3 h-5 w-5 text-white" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                  <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                  <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                </svg>
                Registracija...
              </>
            ) : (
              'Registruj se'
            )}
          </button>
        </form>
      </div>
    </div>
  );
};

export default Register;
