// ui/src/App.tsx (DEFINITIVE, FINAL, CORRECTED, AND COMPLETE VERSION 3.0)

import React, { useState, useEffect, useCallback, useMemo } from 'react';
import axios from 'axios';
import { LineChart, BarChart, Bar, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import './App.css';

// --- Configuration & Types ---
const API_URL = 'http://127.0.0.1:8000';
const POLLING_INTERVAL = 1500;
type SweepParameter = "S0" | "K" | "T" | "r" | "sigma" | "num_paths" | "num_steps";
const allSweepableParams: SweepParameter[] = ["S0", "K", "T", "r", "sigma", "num_paths", "num_steps"];

interface IResult { backend: string; inputs: any; outputs: { price: number; time_ms: number; }; }
interface ITaskStatus { status: 'PENDING' | 'RUNNING' | 'COMPLETED' | 'FAILED'; results: IResult[] | null; progress?: string; }
interface IBackend { key: string; name: string; }

// --- Main App Component ---
function App() {
    // --- State Management ---
    const [activeTab, setActiveTab] = useState<'setup' | 'results'>('setup');
    const [backends, setBackends] = useState<IBackend[]>([]);
    const [selectedBackends, setSelectedBackends] = useState<Set<string>>(new Set());
    
    // Form State
    const [optionParams, setOptionParams] = useState({ S0: 100, K: 105, T: 1.0, r: 0.05, sigma: 0.2 });
    const [simParams, setSimParams] = useState({ num_paths: 102400, num_steps: 100, seed: 42 });
    const [sweep, setSweep] = useState({ parameter: 'num_paths' as SweepParameter, start: 51200, end: 512000, steps: 5 });
    const [enableSweep, setEnableSweep] = useState(true);

    // Task & Results State
    const [task, setTask] = useState<ITaskStatus | null>(null);
    const [taskId, setTaskId] = useState<string | null>(null);
    const [error, setError] = useState<string | null>(null);

    // --- API Calls & Effects ---
    useEffect(() => {
        axios.get<IBackend[]>(`${API_URL}/backends`).then(res => {
            setBackends(res.data);
            const defaultSelection = new Set(res.data.filter(b => ['cpp_mp', 'cpp_ultimate'].includes(b.key)).map(b => b.key));
            setSelectedBackends(defaultSelection);
        }).catch(err => setError("Could not fetch backends from API. Is the server running?"));
    }, []);

    useEffect(() => {
        if (taskId && task?.status !== 'COMPLETED' && task?.status !== 'FAILED') {
            const interval = setInterval(async () => {
                try {
                    const { data } = await axios.get<ITaskStatus>(`${API_URL}/task-status/${taskId}`);
                    setTask(data);
                    if (data.status === 'COMPLETED' || data.status === 'FAILED') {
                        clearInterval(interval);
                        if (data.status === 'COMPLETED') setActiveTab('results');
                    }
                } catch { setError("Failed to get task status."); clearInterval(interval); }
            }, POLLING_INTERVAL);
            return () => clearInterval(interval);
        }
    }, [taskId, task]);

    // --- Event Handlers ---
    const handleRunExperiment = async () => {
        const config = {
            option_params: optionParams,
            simulation_params: simParams,
            backends: Array.from(selectedBackends),
            sweep: enableSweep ? sweep : null
        };
        setError(null);
        try {
            const { data } = await axios.post<{ task_id: string }>(`${API_URL}/run-experiment`, config);
            setTaskId(data.task_id);
            setTask({ status: 'PENDING', results: null });
            setActiveTab('results');
        } catch (error: any) {
            if (error.response && error.response.data && error.response.data.detail) {
                const firstError = error.response.data.detail[0];
                const errorLocation = firstError.loc.join(' -> ');
                setError(`Validation Error: ${firstError.msg} (in ${errorLocation})`);
            } else {
                setError("An unknown error occurred while starting the experiment.");
            }
        }
    };
    
    const handleFormChange = (e: React.ChangeEvent<HTMLInputElement | HTMLSelectElement>, category: 'option' | 'sim' | 'sweep') => {
        const { name, value } = e.target;
        
        if (category === 'sweep' && name === 'parameter') {
            setSweep(p => ({ ...p, parameter: value as SweepParameter }));
            return;
        }

        const numValue = value === '' ? 0 : parseFloat(value);
        const intValue = value === '' ? 0 : parseInt(value, 10);
        const isInt = name === 'steps' || name.startsWith('num_') || name === 'seed';
        const finalValue = isInt ? intValue : numValue;

        if (category === 'option') setOptionParams(p => ({ ...p, [name]: finalValue }));
        if (category === 'sim') setSimParams(p => ({ ...p, [name]: finalValue }));
        if (category === 'sweep') setSweep(p => ({ ...p, [name]: finalValue }));
    };

    const handleBackendSelection = (backendKey: string) => {
        const newSelection = new Set(selectedBackends);
        if (newSelection.has(backendKey)) newSelection.delete(backendKey);
        else newSelection.add(backendKey);
        setSelectedBackends(newSelection);
    };

    const downloadCSV = () => {
        if (!task?.results) return;
        const headers = ["backend", ...Object.keys(task.results[0].inputs), "price", "time_ms"];
        const rows = task.results.map(r => [
            r.backend, ...Object.values(r.inputs), r.outputs.price, r.outputs.time_ms
        ].join(","));
        const csvContent = "data:text/csv;charset=utf-8," + [headers.join(","), ...rows].join("\n");
        const link = document.createElement("a");
        link.setAttribute("href", encodeURI(csvContent));
        link.setAttribute("download", "experiment_results.csv");
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
    };
    
    // --- Memoized Data Transformations ---
    const processedSweepData = useMemo(() => {
        if (!task?.results || !enableSweep) return [];
        const pivotedData: { [key: string]: any } = {};
        const sweepParam = sweep.parameter;

        for (const result of task.results) {
            const xValue = result.inputs[sweepParam];
            if (!pivotedData[xValue]) {
                pivotedData[xValue] = { [sweepParam]: xValue };
            }
            pivotedData[xValue][`${result.backend}_time`] = result.outputs.time_ms;
            pivotedData[xValue][`${result.backend}_price`] = result.outputs.price;
        }
        return Object.values(pivotedData).sort((a, b) => a[sweepParam] - b[sweepParam]);
    }, [task, enableSweep, sweep.parameter]);

    const barChartData = useMemo(() => {
        if (!task?.results || enableSweep) return [];
        return Array.from(selectedBackends).map(key => {
            const backendInfo = backends.find(b => b.key === key);
            const result = task.results?.find(r => r.backend === key);
            return {
                name: backendInfo?.name || key,
                'Time (ms)': result?.outputs.time_ms,
                'Price ($)': result?.outputs.price,
            };
        }).sort((a, b) => (a['Time (ms)'] || 0) - (b['Time (ms)'] || 0));
    }, [task, selectedBackends, backends, enableSweep]);

    const isRunning = task?.status === 'RUNNING' || task?.status === 'PENDING';

    // --- UI Rendering ---
    return (
        <div className="App">
            <header className="App-header"><h1>LSM Performance Workbench</h1></header>
            <div className="tabs">
                <button onClick={() => setActiveTab('setup')} disabled={activeTab === 'setup'}>1. Setup Experiment</button>
                <button onClick={() => setActiveTab('results')} disabled={activeTab === 'results' || !task}>2. View Results</button>
            </div>
            <main className="App-container">
                {activeTab === 'setup' && (
                    <div id="setup-panel">
                        <div className="grid-container">
                             <div className="panel config-panel">
                                <h3>Option Parameters</h3>
                                {Object.entries(optionParams).map(([key, value]) => <label key={key}>{key}: <input type="number" step="any" name={key} value={value} onChange={e => handleFormChange(e, 'option')} disabled={enableSweep && sweep.parameter === key} /></label>)}
                            </div>
                            <div className="panel config-panel">
                                <h3>Simulation Parameters</h3>
                                {Object.entries(simParams).map(([key, value]) => <label key={key}>{key}: <input type="number" name={key} value={value} onChange={e => handleFormChange(e, 'sim')} step={key.startsWith('num_') ? 1 : undefined} disabled={enableSweep && sweep.parameter === key} /></label>)}
                            </div>
                            <div className="panel config-panel full-width">
                                <h3>Backends to Compare</h3>
                                <div className="backend-selector">{backends.map(b => <label key={b.key}><input type="checkbox" checked={selectedBackends.has(b.key)} onChange={() => handleBackendSelection(b.key)} /> {b.name}</label>)}</div>
                            </div>
                            <div className="panel config-panel full-width">
                                <h3>Parameter Sweep</h3>
                                <label className="sweep-toggle"><input type="checkbox" checked={enableSweep} onChange={() => setEnableSweep(!enableSweep)} /> Enable Parameter Sweep</label>
                                {enableSweep && <div className="sweep-controls">
                                     <p className="info-text">The parameter selected below will be swept across the defined range. Its single value in the panels above will be ignored.</p>
                                    <label>Parameter: <select name="parameter" value={sweep.parameter} onChange={e => handleFormChange(e, 'sweep')}>{allSweepableParams.map(p => <option key={p} value={p}>{p}</option>)}</select></label>
                                    <label>Start: <input type="number" step="any" name="start" value={sweep.start} onChange={e => handleFormChange(e, 'sweep')} /></label>
                                    <label>End: <input type="number" step="any" name="end" value={sweep.end} onChange={e => handleFormChange(e, 'sweep')} /></label>
                                    <label>Steps: <input type="number" name="steps" value={sweep.steps} onChange={e => handleFormChange(e, 'sweep')} min={2} step={1} /></label>
                                </div>}
                            </div>
                        </div>
                        <div className="run-container"><button onClick={handleRunExperiment} disabled={isRunning || selectedBackends.size === 0}>{isRunning ? `Running: ${task?.progress || '...'}` : 'Run Experiment'}</button>{error && <p className="error-message">{error}</p>}</div>
                    </div>
                )}
                {activeTab === 'results' && (
                    <div id="results-panel">
                        {!task ? <p>No experiment has been run yet.</p> :
                         isRunning ? <p className="status-message">Running: {task.progress || 'Please wait...'}</p> :
                         task.status === 'FAILED' ? <p className="error-message">Experiment Failed! Check API logs.</p> :
                         task.status === 'COMPLETED' && task.results && task.results.length > 0 ? (
                            <>
                                <div className="results-controls"><h3>{enableSweep ? `Sweep of '${sweep.parameter}'` : 'Single Run Comparison'}</h3><button onClick={downloadCSV}>Download Results as CSV</button></div>
                                <ResponsiveContainer width="100%" height={500}>
                                    {enableSweep ? (
                                        <LineChart data={processedSweepData} margin={{ top: 20, right: 30, left: 20, bottom: 100 }}>
                                            <CartesianGrid strokeDasharray="3 3" />
                                            <XAxis dataKey={sweep.parameter} type="number" domain={['dataMin', 'dataMax']} label={{ value: sweep.parameter, position: 'insideBottom', offset: -80 }}/>
                                            <YAxis yAxisId="left" label={{ value: 'Time (ms)', angle: -90, position: 'insideLeft' }} />
                                            <YAxis yAxisId="right" orientation="right" label={{ value: 'Price ($)', angle: 90, position: 'insideRight' }} />
                                            <Tooltip />
                                            <Legend verticalAlign="top" wrapperStyle={{ paddingBottom: '20px' }}/>
                                            {Array.from(selectedBackends).map((key, i) => <Line key={`${key}-time`} yAxisId="left" type="monotone" dataKey={`${key}_time`} name={`${backends.find(b => b.key === key)?.name} Time`} strokeWidth={2} stroke={`hsl(${(i * 137.5) % 360}, 70%, 45%)`} />)}
                                            {Array.from(selectedBackends).map((key, i) => <Line key={`${key}-price`} yAxisId="right" type="monotone" dataKey={`${key}_price`} name={`${backends.find(b => b.key === key)?.name} Price`} strokeWidth={2} stroke={`hsl(${(i * 137.5 + 60) % 360}, 60%, 65%)`} strokeDasharray="5 5"/>)}
                                        </LineChart>
                                    ) : (
                                        <BarChart data={barChartData} margin={{ top: 20, right: 30, left: 20, bottom: 120 }}>
                                            <CartesianGrid strokeDasharray="3 3" />
                                            <XAxis dataKey="name" angle={-45} textAnchor="end" interval={0} height={100}/>
                                            <YAxis yAxisId="left" label={{ value: 'Time (ms)', angle: -90, position: 'insideLeft' }}/>
                                            <YAxis yAxisId="right" orientation="right" label={{ value: 'Price ($)', angle: 90, position: 'insideRight' }}/>
                                            <Tooltip />
                                            <Legend verticalAlign="top" wrapperStyle={{ paddingBottom: '20px' }}/>
                                            <Bar yAxisId="left" dataKey="Time (ms)" fill="#8884d8" />
                                            <Bar yAxisId="right" dataKey="Price ($)" fill="#82ca9d" />
                                        </BarChart>
                                    )}
                                </ResponsiveContainer>
                            </>
                         ) : <p>Experiment completed but no results were returned.</p>
                        }
                    </div>
                )}
            </main>
        </div>
    );
}

export default App;