import React from 'react';
import { Link } from 'react-router-dom';

const NotFound = () => {
  return (
    <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 flex items-center justify-center">
      <div className="text-center bg-white/10 backdrop-blur-lg rounded-xl p-8 max-w-md w-full shadow-2xl border border-white/20">
        <h1 className="text-4xl font-bold text-white mb-4">404</h1>
        <p className="text-xl text-white/90 mb-6">Page not found</p>
        <Link 
          to="/" 
          className="px-6 py-2 bg-emerald-600 text-white rounded-lg font-medium 
            hover:bg-emerald-700 transition-colors duration-300 inline-block"
        >
          Go Home
        </Link>
      </div>
    </div>
  );
};

export default NotFound; 