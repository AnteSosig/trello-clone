import React from 'react';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import Navbar from './components/Navbar';
import Home from './pages/Home';
import LoginForm from './components/Login';
import RegisterUser from './pages/RegisterUser';
import SearchFilter from './components/SearchFilter';
import NotFound from './pages/NotFound';
import EditProject from './pages/EditProject';
import AccountConfirm from './pages/AccountConfirm';
import Profile from './pages/Profile';
import ChangePassword from './pages/ChangePassword';

function App() {
  return (
    <Router>
      <Navbar />
      <Routes>
        <Route path="/" element={<Home />} />
        <Route path="/login" element={<LoginForm />} />
        <Route path="/register" element={<RegisterUser />} />
        <Route path="/pretraga" element={<SearchFilter />} />
        <Route path="/edit-project/:id" element={<EditProject />} />
        <Route path="/profile" element={<Profile />} />
        <Route path="/change-password" element={<ChangePassword />} />
        <Route path="*" element={<NotFound />} />
        <Route path="/activate" element={<AccountConfirm />} />
      </Routes>
    </Router>
  );
}

export default App;
