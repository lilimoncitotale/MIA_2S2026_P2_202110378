import React, { useState, useEffect } from 'react';

export default function Home() {
  const [comando, setComando] = useState('');
  const [respuesta, setRespuesta] = useState('');
  const [cargando, setCargando] = useState(false);
  const [conectado, setConectado] = useState(false);

  const probarConexion = async () => {
    try {
      const response = await fetch('http://localhost:8080/ping');
      const data = await response.json();
      setConectado(true);
      setRespuesta(`✅ Conectado: ${data.message}`);
    } catch (error) {
      setConectado(false);
      setRespuesta('❌ Error de conexión: No se pudo conectar al servidor');
      console.error('Error:', error);
    }
  };

  const enviarComando = async () => {
    if (!comando.trim()) {
      setRespuesta('⚠️ Por favor, ingresa un comando');
      return;
    }

    setCargando(true);
    setRespuesta('⏳ Procesando...');

    try {
      const lineas = comando.split('\n').filter(line => line.trim() !== '');

      if (lineas.length === 1) {
        const response = await fetch('http://localhost:8080/comando', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ comando: lineas[0].trim() }),
        });
        const data = await response.json();

        if (data.success) {
          setRespuesta(`${data.resultado}`);
        } else {
          setRespuesta(`❌ ${data.errores?.join('\n') || 'Error desconocido'}`);
        }
      } else {
        let resultados = '📦 PROCESANDO MÚLTIPLES COMANDOS\n';
        resultados += '━'.repeat(50) + '\n\n';
        let errores = 0;

        for (let i = 0; i < lineas.length; i++) {
          const cmd = lineas[i].trim();
          resultados += `${i + 1}. ${cmd}\n`;

          try {
            const response = await fetch('http://localhost:8080/comando', {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify({ comando: cmd }),
            });
            const data = await response.json();

            if (data.success) {
              resultados += `    ${data.resultado.split('\n').join('\n    ')}\n\n`;
            } else {
              resultados += `    ${data.errores?.join('\n') || 'Error desconocido'}\n\n`;
              errores++;
            }
          } catch (error) {
            resultados += `    Error de conexión\n\n`;
            errores++;
          }
        }

        resultados += '━'.repeat(50) + '\n';
        resultados += `📊 Resumen: ${lineas.length - errores} comandos exitosos, ${errores} con errores`;

        setRespuesta(resultados);
      }
    } catch (error) {
      setRespuesta('❌ Error de conexión con el servidor');
      console.error('Error:', error);
    }

    setCargando(false);
  };

  const analizarComando = async () => {
    if (!comando.trim()) {
      setRespuesta('⚠️ Por favor, ingresa un comando');
      return;
    }

    setCargando(true);
    setRespuesta('⏳ Analizando...');

    try {
      const response = await fetch('http://localhost:8080/analizar', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ comando: comando }),
      });

      const data = await response.json();

      let resultado = '🔍 TOKENS ENCONTRADOS:\n';
      resultado += '━'.repeat(50) + '\n\n';

      if (data.tokens && data.tokens.length > 0) {
        data.tokens.forEach(token => {
          const tipo = token.tipo || 'DESCONOCIDO';
          const valor = token.valor || '';
          const linea = token.linea || 1;
          const columna = token.columna || 1;
          resultado += `  ${tipo.padEnd(12)} | "${valor}"  (línea ${linea}, col ${columna})\n`;
        });
      } else {
        resultado += '  (No se encontraron tokens)\n';
      }

      resultado += '\n' + '━'.repeat(50) + '\n';
      resultado += `Total tokens: ${data.stats?.totalTokens || 0}\n`;

      if (data.errores && data.errores.length > 0) {
        resultado += '\n❌ ERRORES LÉXICOS:\n';
        data.errores.forEach(error => {
          resultado += `   ${error}\n`;
        });
      } else {
        resultado += '\n✅ Sin errores léxicos';
      }

      setRespuesta(resultado);
    } catch (error) {
      setRespuesta('❌ Error de conexión con el servidor');
      console.error('Error:', error);
    }

    setCargando(false);
  };

  useEffect(() => {
    probarConexion();
  }, []);

  return (
    <div className="app">
      <h1>🚀 Analizador de Comandos EXT2</h1>

      <div className="conexion-status">
        <span className={conectado ? 'conectado' : 'desconectado'}>
          {conectado ? '🟢 Conectado al servidor' : '🔴 Desconectado'}
        </span>
        <button onClick={probarConexion} className="btn-test">
          Probar conexión
        </button>
      </div>

      <div className="input-area">
        <label htmlFor="comando">Ingresa un comando:</label>
        <div className="input-group">
          <textarea
            id="comando"
            value={comando}
            onChange={(e) => setComando(e.target.value)}
            placeholder="Ej: mkdisk -size=10 -path=/home/Disco1.mia&#10;fdisk -size=30 -path=/home/Disco1.mia -name=part1&#10;mount -path=/home/Disco1.mia -name=part1"
            rows="5"
            style={{ fontFamily: 'monospace' }}
          />
          <div style={{ display: 'flex', flexDirection: 'column', gap: '8px' }}>
            <button
              onClick={enviarComando}
              disabled={cargando || !conectado}
              className="btn-enviar"
            >
              {cargando ? '⏳ Enviando...' : '▶️ Ejecutar'}
            </button>
            <button
              onClick={analizarComando}
              disabled={cargando || !conectado}
              className="btn-enviar"
              style={{ background: '#28a745' }}
            >
              {cargando ? '⏳ Analizando...' : '🔍 Analizar Léxico'}
            </button>
          </div>
        </div>
      </div>

      <div className="output-area">
        <h3>📤 Resultado:</h3>
        <div className="output-box">
          {respuesta || 'Esperando comando...'}
        </div>
      </div>
    </div>
  );
}