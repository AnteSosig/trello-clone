import React, { useState } from 'react';
import axios from 'axios';

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

  const handleChange = (e) => {
    setFormData({ ...formData, [e.target.name]: e.target.value });
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    try {
      const userPayload = {
        username: formData.username,
        first_name: formData.first_name,
        last_name: formData.last_name,
        email: formData.email,
        password: formData.password,
        role: formData.role
      };

      const response = await axios.post('https://localhost:8443/user/newuser', userPayload, {
        headers: {
          'Content-Type': 'application/json'
        }
      });

      setSuccess('Uspešno ste se registrovali!');
      setError('');
    } catch (error) {
      setError('Došlo je do greške prilikom registracije. Molimo pokušajte ponovo.');
      setSuccess('');
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
            <input
              type="password"
              id="password"
              name="password"
              value={formData.password}
              onChange={handleChange}
              className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg 
                       text-white placeholder-white/50 focus:outline-none focus:ring-2 
                       focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg"
              required
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
            className="w-full px-4 py-2 bg-emerald-600 text-white rounded-lg 
                     hover:bg-emerald-700 transition-colors duration-200 font-medium"
          >
            Registruj se
          </button>
        </form>
      </div>
    </div>
  );
};

export default Register;
