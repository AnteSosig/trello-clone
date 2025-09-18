import React from 'react';
import { Link } from 'react-router-dom';

const Profile = () => {
  return (
    <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 py-12 px-4 sm:px-6 lg:px-8">
      <div className="max-w-4xl mx-auto">
        <div className="bg-white/10 backdrop-blur-lg rounded-xl p-8 shadow-2xl border border-white/20">
          <h1 className="text-3xl font-bold text-white mb-8">Moj Profil</h1>
          <div className="text-center py-12">
            <p className="text-white/80 text-lg mb-8">Ova stranica je u razvoju...</p>
            <Link
              to="/change-password"
              className="inline-block px-6 py-3 bg-emerald-600 text-white rounded-lg font-medium 
                hover:bg-emerald-700 transition-colors duration-300"
            >
              Promeni lozinku
            </Link>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Profile;
