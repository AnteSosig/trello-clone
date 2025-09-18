import React, { useState } from 'react';
import { Link, useNavigate } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';

const Navbar = () => {
  const [isOpen, setIsOpen] = useState(false);
  const navigate = useNavigate();
  const { isAuthenticated, user, logout } = useAuth();

  const handleLogout = () => {
    logout();
    navigate('/login');
  };

  return (
    <nav className="bg-white border-b shadow-lg sticky top-0 z-50">
      <div className="container mx-auto px-4">
        <div className="flex justify-between items-center h-20">
          {/* Logo/Brand */}
          <Link to="/" className="flex items-center space-x-3 no-underline hover:no-underline">
            <svg
              className="w-8 h-8 text-emerald-600"
              fill="none"
              viewBox="0 0 24 24"
              stroke="currentColor"
            >
              <path
                strokeLinecap="round"
                strokeLinejoin="round"
                strokeWidth={2}
                d="M19 11H5m14 0a2 2 0 012 2v6a2 2 0 01-2 2H5a2 2 0 01-2-2v-6a2 2 0 012-2m14 0V9a2 2 0 00-2-2M5 11V9a2 2 0 012-2m0 0V5a2 2 0 012-2h6a2 2 0 012 2v2M7 7h10"
              />
            </svg>
            <span className="text-2xl font-bold bg-gradient-to-r from-emerald-600 to-teal-500 text-transparent bg-clip-text">
              Trelloc
            </span>
          </Link>

          {/* Mobile menu button */}
          <button
            onClick={() => setIsOpen(!isOpen)}
            className="md:hidden text-gray-700 hover:text-emerald-600 focus:outline-none"
          >
            <svg className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              {isOpen ? (
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
              ) : (
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 6h16M4 12h16M4 18h16" />
              )}
            </svg>
          </button>

          {/* Desktop Navigation */}
          <div className="hidden md:flex items-center space-x-1">
            {!isAuthenticated ? (
              <>
                <NavLink to="/login">Prijavi se</NavLink>
                <NavLink to="/register">Registracija</NavLink>
              </>
            ) : (
              <>
                <span className="px-4 py-2 text-gray-700 font-medium">
                  Role: {user?.role}
                </span>
                <button
                  onClick={handleLogout}
                  className="px-4 py-2 text-gray-700 hover:text-emerald-600 rounded-lg hover:bg-gray-50 
                             transition-all duration-200 font-medium"
                >
                  Logout
                </button>
              </>
            )}
          </div>
        </div>

        {/* Mobile Navigation */}
        <div className={`md:hidden ${isOpen ? 'block' : 'hidden'} pb-6`}>
          <div className="flex flex-col space-y-2">
            {!isAuthenticated ? (
              <>
                <MobileNavLink to="/login">Prijavi se</MobileNavLink>
                <MobileNavLink to="/register">Registracija</MobileNavLink>
              </>
            ) : (
              <>
                <span className="block px-4 py-2 text-gray-700 font-medium">
                  Role: {user?.role}
                </span>
                <button
                  onClick={handleLogout}
                  className="block px-4 py-2 text-gray-700 hover:text-emerald-600 hover:bg-gray-50 
                             transition-colors duration-200 text-left"
                >
                  Logout
                </button>
              </>
            )}
          </div>
        </div>
      </div>
    </nav>
  );
};

const NavLink = ({ to, children }) => (
  <Link
    to={to}
    className="px-4 py-2 text-gray-700 hover:text-emerald-600 rounded-lg hover:bg-gray-50 
              transition-all duration-200 font-medium no-underline hover:no-underline"
  >
    {children}
  </Link>
);

const MobileNavLink = ({ to, children }) => (
  <Link
    to={to}
    className="block px-4 py-2 text-gray-700 hover:text-emerald-600 hover:bg-gray-50 
              transition-colors duration-200 no-underline hover:no-underline"
  >
    {children}
  </Link>
);

export default Navbar;
