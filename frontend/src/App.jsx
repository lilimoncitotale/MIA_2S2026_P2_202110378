import React from 'react';
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import { SessionProvider, useSession } from './context/SessionContext';
import NavBar from './components/NavBar';
import Home from './pages/Home';
import Login from './pages/Login';
import DiskSelector from './pages/DiskSelector';
import PartitionSelector from './pages/PartitionSelector';
import FileExplorer from './pages/FileExplorer';
import Journal from './pages/Journal';
import './App.css';

function ProtectedRoute({ children }) {
  const { session, loading } = useSession();
  if (loading) return <div>Cargando...</div>;
  if (!session?.active) return <Navigate to="/login" />;
  return children;
}

function App() {
  return (
    <SessionProvider>
      <BrowserRouter>
        <NavBar />
        <Routes>
          <Route path="/" element={<Home />} />
          <Route path="/login" element={<Login />} />
          <Route path="/visualizador" element={<DiskSelector />} />
          <Route path="/particiones" element={<PartitionSelector />} />
          <Route path="/explorador" element={<FileExplorer />} />
          <Route path="/journal" element={<Journal />} />
          <Route path="*" element={<Navigate to="/" />} />
        </Routes>
      </BrowserRouter>
    </SessionProvider>
  );
}

export default App;