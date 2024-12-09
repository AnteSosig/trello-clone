import React, { useState, useEffect } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import Cookies from 'js-cookie';
import { jwtDecode } from "jwt-decode";

const AddTasks = () => {
  const { projectId } = useParams();
  const navigate = useNavigate();
  const [projectData, setProjectData] = useState(null);
  const [tasks, setTasks] = useState([]);
  const [userRole, setUserRole] = useState(null);
  const [taskData, setTaskData] = useState({
    name: '',
    description: '',
    members: [],
    status: "pending"
  });
  const [error, setError] = useState('');
  const [success, setSuccess] = useState('');
  const [showAddTaskModal, setShowAddTaskModal] = useState(false);

  useEffect(() => {
    const fetchData = async () => {
      try {
        const role = Cookies.get('role');
        setUserRole(role);

        const projectResponse = await fetch(`http://localhost:8081/projects/${projectId}`);
        const projectData = await projectResponse.json();
        setProjectData(projectData);

        const tasksResponse = await fetch(`http://localhost:8082/tasks/project/${projectId}`);
        const tasksData = await tasksResponse.json();
        setTasks(tasksData);
      } catch (err) {
        console.error('Error fetching data:', err);
        setError('Failed to fetch project data');
      }
    };

    fetchData();
  }, [projectId]);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError('');
    setSuccess('');

    try {
      const tokenCookie = document.cookie
        .split('; ')
        .find(row => row.startsWith('token='));
      
      if (!tokenCookie) {
        setError('Authentication required');
        return;
      }

      const token = tokenCookie.split('=')[1];
      const decodedToken = jwtDecode(token);

      const STATUS = {
        STATUS_PENDING: 0,
        STATUS_IN_PROGRESS: 1,
        STATUS_COMPLETED: 2
      };

      const taskPayload = {
        name: taskData.name,
        description: taskData.description,
        members: taskData.members,
        status: STATUS.STATUS_PENDING,
        project_id: projectId,
        creator_id: decodedToken.sub
      };

      console.log('Sending task payload:', taskPayload);

      const response = await fetch('http://localhost:8082/tasks', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(taskPayload),
      });

      if (!response.ok) {
        throw new Error('Failed to create task');
      }

      const tasksResponse = await fetch(`http://localhost:8082/tasks/project/${projectId}`);
      const tasksData = await tasksResponse.json();
      setTasks(tasksData);

      setSuccess('Task created successfully');
      setTaskData({
        name: '',
        description: '',
        members: [],
        status: STATUS.STATUS_PENDING
      });

    } catch (err) {
      console.error('Error creating task:', err);
      setError(err.message);
    }
  };

  const handleMemberSelection = (member) => {
    setTaskData((prevData) => {
      const isSelected = prevData.members.includes(member);
      const newMembers = isSelected
        ? prevData.members.filter((m) => m !== member)
        : [...prevData.members, member];
      return { ...prevData, members: newMembers };
    });
  };

  const handleStatusChange = async (taskId, newStatus) => {
    try {
      const response = await fetch(`http://localhost:8082/tasks/${taskId}/status`, {
        method: 'PATCH',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ status: newStatus }),
      });

      if (!response.ok) {
        throw new Error('Failed to update task status');
      }

      const tasksResponse = await fetch(`http://localhost:8082/tasks/project/${projectId}`);
      const tasksData = await tasksResponse.json();
      setTasks(tasksData);

    } catch (err) {
      console.error('Error updating task status:', err);
      setError(err.message);
    }
  };

  // Define a mapping for status codes to strings
  const STATUS_MAP = {
    0: "pending",
    1: "in progress",
    2: "completed"
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 py-12 px-4">
      <div className="max-w-6xl mx-auto">
        <div className="bg-white/10 backdrop-blur-lg rounded-xl p-8 shadow-2xl mb-8">
          <h2 className="text-3xl font-bold text-white mb-6">{projectData?.project}</h2>
          <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
            <div>
              <p className="text-white/90 mb-2">
                <span className="font-semibold">Description:</span><br />
                {projectData?.description}
              </p>
              <p className="text-white/90">
                <span className="font-semibold">Moderator:</span> {projectData?.moderator}
              </p>
              <p className="text-white/90">
                <span className="font-semibold">Estimated Completion:</span><br />
                {projectData?.estimated_completion_date}
              </p>
              
              {userRole === 'MANAGER' && (
                <div className="mt-4">
                  <button
                    onClick={() => setShowAddTaskModal(true)}
                    className="px-6 py-2 bg-emerald-600 text-white rounded-lg hover:bg-emerald-700 transition-colors flex items-center gap-2"
                  >
                    <svg xmlns="http://www.w3.org/2000/svg" className="h-5 w-5" viewBox="0 0 20 20" fill="currentColor">
                      <path fillRule="evenodd" d="M10 3a1 1 0 011 1v5h5a1 1 0 110 2h-5v5a1 1 0 11-2 0v-5H4a1 1 0 110-2h5V4a1 1 0 011-1z" clipRule="evenodd" />
                    </svg>
                    Add New Task
                  </button>
                </div>
              )}
            </div>
            <div>
              <p className="text-white/90">
                <span className="font-semibold">Members ({projectData?.current_member_count}/{projectData?.max_members}):</span>
              </p>
              <ul className="list-disc list-inside text-white/90">
                {projectData?.members.map((member, index) => (
                  <li key={index}>{member}</li>
                ))}
              </ul>
            </div>
          </div>
        </div>
         <div className="bg-white/10 backdrop-blur-lg rounded-xl p-8 shadow-2xl">
          <h3 className="text-2xl font-bold text-white mb-6">Current Tasks</h3>
          <div className="grid gap-4">
            {tasks.length > 0 ? (
              tasks.map((task, index) => (
                <div key={index} className="bg-white/5 rounded-lg p-4 border border-white/10">
                  <h4 className="text-xl font-semibold text-white mb-2">{task.name}</h4>
                  <p className="text-white/90 mb-2">{task.description}</p>
                  <div className="flex justify-between items-center">
                    <div className="text-white/80">
                      <span className="font-semibold">Status:</span> {STATUS_MAP[task.status]}
                      {/* {userRole === 'USER' && (
                        <select
                          value={task.status}
                          onChange={(e) => handleStatusChange(task._id, parseInt(e.target.value))}
                          className="ml-2 bg-white/10 border border-white/20 rounded-lg text-white"
                        >
                          <option value={0}>Pending</option>
                          <option value={1}>In Progress</option>
                          <option value={2}>Completed</option>
                        </select>
                      )} */}
                    </div>
                    <div className="text-white/80">
                      <span className="font-semibold">Assigned to:</span>{' '}
                      {task.members.join(', ')}
                    </div>
                  </div>
                </div>
              ))
            ) : (
              <p className="text-white/90">No tasks created yet.</p>
            )}
          </div>
        </div>
        {showAddTaskModal && userRole === 'MANAGER' && (
          <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50">
            <div className="bg-white/10 backdrop-blur-lg rounded-xl p-8 max-w-2xl w-full shadow-2xl border border-white/20">
              <div className="flex justify-between items-start mb-6">
                <h2 className="text-2xl font-bold text-white">Add New Task</h2>
                <button 
                  onClick={() => setShowAddTaskModal(false)}
                  className="text-white/80 hover:text-white"
                >
                  ✕
                </button>
              </div>

              {error && (
                <div className="mb-4 p-4 bg-red-500/20 backdrop-blur-lg border border-red-500/50 rounded-lg text-white">
                  {error}
                </div>
              )}

              {success && (
                <div className="mb-4 p-4 bg-green-500/20 backdrop-blur-lg border border-green-500/50 rounded-lg text-white">
                  {success}
                </div>
              )}

              <form onSubmit={handleSubmit} className="space-y-6">
                <div>
                  <label className="block text-white mb-2">Task Name</label>
                  <input
                    type="text"
                    value={taskData.name}
                    onChange={(e) => setTaskData({...taskData, name: e.target.value})}
                    className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg text-white"
                    required
                  />
                </div>

                <div>
                  <label className="block text-white mb-2">Description</label>
                  <textarea
                    value={taskData.description}
                    onChange={(e) => setTaskData({...taskData, description: e.target.value})}
                    className="w-full px-4 py-2 bg-white/10 border border-white/20 rounded-lg text-white h-32"
                    required
                  />
                </div>

                <div>
                  <label className="block text-white mb-2">Assign Members</label>
                  <div className="grid grid-cols-2 gap-2">
                    {projectData?.members.map((member) => (
                      <label key={member} className="flex items-center text-white">
                        <input
                          type="checkbox"
                          checked={taskData.members.includes(member)}
                          onChange={() => handleMemberSelection(member)}
                          className="mr-2"
                        />
                        {member}
                      </label>
                    ))}
                  </div>
                </div>

                <div className="flex justify-end gap-4">
                  <button
                    type="button"
                    onClick={() => setShowAddTaskModal(false)}
                    className="px-6 py-2 bg-white/20 text-white rounded-lg hover:bg-white/30"
                  >
                    Cancel
                  </button>
                  <button
                    type="submit"
                    className="px-6 py-2 bg-emerald-600 text-white rounded-lg hover:bg-emerald-700"
                  >
                    Create Task
                  </button>
                </div>
              </form>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};

export default AddTasks; 