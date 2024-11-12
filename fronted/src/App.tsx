import React from 'react';
import { BrowserRouter, Routes, Route } from 'react-router-dom';
import './App.css';
import Box from './components/Box/Box.tsx';
import EditMembers from './components/EditMembers/EditMembers.tsx';

function App() {
  return (
    <BrowserRouter>
      <div className="App">
        <header className="App-header">
          <Routes>
            <Route path="/" element={<Box />} />
            <Route path="/edit-members/:projectId" element={<EditMembers />} />
          </Routes>
        </header>
      </div>
    </BrowserRouter>
  );
}

export default App;
