import React, { createContext, useContext, useState, useEffect } from 'react';
import { estadoSesion, loginGrafico, logoutGrafico } from '../api';

const SessionContext = createContext();

export function SessionProvider({ children }) {
  const [session, setSession] = useState(null);
  const [loading, setLoading] = useState(true);

  // Verificar sesión al inicio
  useEffect(() => {
    async function check() {
      try {
        const data = await estadoSesion();
        if (data.active) setSession(data);
      } catch (e) {
        console.error('Error al verificar sesión:', e);
      }
      setLoading(false);
    }
    check();
  }, []);

  const login = async (id, user, pass) => {
    const data = await loginGrafico(id, user, pass);
    if (data.success) {
      setSession({
        active: true,
        user: data.user,
        uid: data.uid,
        gid: data.gid,
        id: data.id
      });
      return true;
    }
    return false;
  };

  const logout = async () => {
    await logoutGrafico();
    setSession(null);
  };

  return (
    <SessionContext.Provider value={{ session, login, logout, loading }}>
      {children}
    </SessionContext.Provider>
  );
}

export function useSession() {
  return useContext(SessionContext);
}