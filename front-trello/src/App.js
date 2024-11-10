import React from 'react';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import Navbar from './components/Navbar';
import Home from './pages/Home';
import LoginForm from './components/Login';
import RegisterUser from './pages/RegisterUser';
import SearchFilter from './components/SearchFilter';

function App() {
  return (
    <Router>
      <Navbar />
      <Routes>
        <Route path="/" element={<Home />} />
        <Route path="/login" element={<LoginForm />} />
        <Route path="/register" element={<RegisterUser />} />
        <Route path="/pretraga" element={<SearchFilter />} />
      </Routes>
    </Router>
  );
}

export default App;
