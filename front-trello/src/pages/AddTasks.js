import React, { useState, useEffect } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import { jwtDecode } from "jwt-decode";
import { projectApi, taskApi } from '../utils/axios';
import { useAuth } from '../contexts/AuthContext';
import { ManagerOnly, usePermissions } from '../components/RoleBasedRender';

const AddTasks = () => {
  const { projectId } = useParams();
  const navigate = useNavigate();
  const [projectData, setProjectData] = useState(null);
  const [tasks, setTasks] = useState([]);
  const [taskData, setTaskData] = useState({
    name: '',
    description: '',
    members: [],
    status: "pending"
  });
  const [error, setError] = useState('');
  const [success, setSuccess] = useState('');
  const [showAddTaskModal, setShowAddTaskModal] = useState(false);
  const [showMemberModal, setShowMemberModal] = useState(false);
  const [selectedTaskId, setSelectedTaskId] = useState(null);
  const [memberAction, setMemberAction] = useState(''); // 'add' or 'remove'
  const [selectedMember, setSelectedMember] = useState('');
  const { user, isManager } = useAuth();
  const { permissions } = usePermissions();

  useEffect(() => {
    const fetchData = async () => {
      try {
        if (!user) {
          setError('Authentication required');
          navigate('/login');
          return;
        }

        // Fetch project data with authorization
        const projectResponse = await projectApi.get(`/projects/${projectId}`);
        setProjectData(projectResponse.data);

        // Fetch tasks data with authorization  
        const tasksResponse = await taskApi.get(`/tasks/project/${projectId}`);
        setTasks(tasksResponse.data);
      } catch (err) {
        console.error('Error fetching data:', err);
        if (err.response?.status === 401) {
          setError('Authentication failed - please login again');
          navigate('/login');
        } else if (err.response?.status === 403) {
          setError('Access denied - insufficient permissions');
        } else {
          setError('Failed to fetch project data');
        }
      }
    };

    fetchData();
  }, [projectId, navigate]);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError('');
    setSuccess('');

    try {
      if (!user) {
        setError('Authentication required');
        return;
      }

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
        creator_id: user.id
      };

      console.log('Sending task payload:', taskPayload);

      // Create task with authorization
      await taskApi.post('/tasks', taskPayload);

      // Fetch updated tasks with authorization
      const tasksResponse = await taskApi.get(`/tasks/project/${projectId}`);
      setTasks(tasksResponse.data);

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

  const handleStatusChange = async (taskId, newStatusString) => {
    try {
      // Update task status with authorization
      await taskApi.patch(`/tasks/status/${taskId}`, { status: newStatusString });

      // Refetch tasks to update UI with authorization
      const tasksResponse = await taskApi.get(`/tasks/project/${projectId}`);
      setTasks(tasksResponse.data);

    } catch (err) {
      console.error('Error updating task status:', err);
      if (err.response?.status === 403) {
        setError('Access denied - insufficient permissions to update task status');
      } else {
        setError(err.message);
      }
    }
  };

  const handleAddMember = (taskId) => {
    setSelectedTaskId(taskId);
    setMemberAction('add');
    setSelectedMember('');
    setShowMemberModal(true);
  };

  const handleRemoveMember = (taskId) => {
    setSelectedTaskId(taskId);
    setMemberAction('remove');
    setSelectedMember('');
    setShowMemberModal(true);
  };

  const submitMemberAction = async () => {
    if (!selectedMember || !selectedTaskId) {
      setError('Please select a member');
      return;
    }

    try {
      setError('');
      setSuccess('');

      const endpoint = memberAction === 'add' 
        ? `/tasks/${selectedTaskId}/add-member`
        : `/tasks/${selectedTaskId}/remove-member`;

      await taskApi.post(endpoint, { member_id: selectedMember });

      // Refetch tasks to update UI
      const tasksResponse = await taskApi.get(`/tasks/project/${projectId}`);
      setTasks(tasksResponse.data);

      setSuccess(`Member ${memberAction === 'add' ? 'added to' : 'removed from'} task successfully`);
      setShowMemberModal(false);
      setSelectedTaskId(null);
      setSelectedMember('');

    } catch (err) {
      console.error(`Error ${memberAction}ing member:`, err);
      if (err.response?.status === 400) {
        if (memberAction === 'add') {
          setError('User is not a member of this project');
        } else {
          setError('Cannot remove member from completed task');
        }
      } else if (err.response?.status === 403) {
        setError('Access denied - only managers can modify task members');
      } else {
        setError(`Failed to ${memberAction} member: ${err.response?.data?.error || err.message}`);
      }
    }
  };

  const getAvailableMembers = () => {
    if (!projectData?.members) return [];
    
    if (memberAction === 'add') {
      // For adding: show all project members
      return projectData.members;
    } else {
      // For removing: show only members currently assigned to the task
      const currentTask = tasks.find(task => getTaskId(task) === selectedTaskId);
      return currentTask?.members || [];
    }
  };

  const canModifyMembers = (task) => {
    return isManager && task.status !== 2; // Can modify if manager and task not completed
  };

  // Define a mapping for status codes to strings
  const STATUS_MAP = {
    0: "pending",
    1: "in_progress",
    2: "completed"
  };
  const STATUS_LABELS = {
    pending: "Pending",
    in_progress: "In Progress",
    completed: "Completed"
  };
  const STATUS_OPTIONS = [
    { value: "pending", label: "Pending" },
    { value: "in_progress", label: "In Progress" },
    { value: "completed", label: "Completed" }
  ];

  // Helper to normalize MongoDB _id
  const getTaskId = (task) => typeof task._id === 'object' && task._id.$oid ? task._id.$oid : task._id;

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
              
              <ManagerOnly>
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
              </ManagerOnly>
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
                  <p className="text-white/90 mb-3">{task.description}</p>
                  
                  <div className="flex flex-col md:flex-row md:justify-between md:items-center gap-3">
                    <div className="text-white/80 flex items-center">
                      <span className="font-semibold">Status:</span> {" "}
                      {permissions.canUpdateTaskStatus(task.members, task.creator_id) ? (
                        <select
                          value={STATUS_MAP[task.status]}
                          onChange={(e) => handleStatusChange(getTaskId(task), e.target.value)}
                          className="ml-2 bg-white/10 border border-white/20 rounded-lg text-white focus:outline-none focus:ring-2 focus:ring-emerald-500 focus:border-transparent backdrop-blur-lg [&>option]:text-gray-900 [&>option]:bg-white"
                          style={{ minWidth: 120 }}
                        >
                          {STATUS_OPTIONS.map(opt => (
                            <option key={opt.value} value={opt.value}>{opt.label}</option>
                          ))}
                        </select>
                      ) : (
                        <span className={`ml-2 px-2 py-1 rounded-full text-xs font-medium ${
                          task.status === 2 ? 'bg-green-500/20 text-green-300' :
                          task.status === 1 ? 'bg-yellow-500/20 text-yellow-300' :
                          'bg-gray-500/20 text-gray-300'
                        }`}>
                          {STATUS_LABELS[STATUS_MAP[task.status]]}
                        </span>
                      )}
                    </div>
                    
                    <div className="text-white/80">
                      <span className="font-semibold">Assigned to:</span>{' '}
                      {task.members.length > 0 ? task.members.join(', ') : 'No members assigned'}
                    </div>
                  </div>

                  {/* Member Management Buttons */}
                  {isManager && (
                    <div className="mt-4 pt-3 border-t border-white/10 flex gap-2 flex-wrap">
                      <button
                        onClick={() => handleAddMember(getTaskId(task))}
                        className="px-3 py-1 bg-blue-600/20 text-blue-300 border border-blue-400/30 rounded-lg hover:bg-blue-600/30 transition-colors text-sm flex items-center gap-1"
                      >
                        <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 6v6m0 0v6m0-6h6m-6 0H6" />
                        </svg>
                        Add Member
                      </button>
                      
                      {canModifyMembers(task) && task.members.length > 0 && (
                        <button
                          onClick={() => handleRemoveMember(getTaskId(task))}
                          className="px-3 py-1 bg-red-600/20 text-red-300 border border-red-400/30 rounded-lg hover:bg-red-600/30 transition-colors text-sm flex items-center gap-1"
                        >
                          <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M20 12H4" />
                          </svg>
                          Remove Member
                        </button>
                      )}
                      
                      {task.status === 2 && (
                        <span className="px-3 py-1 bg-gray-600/20 text-gray-400 border border-gray-500/30 rounded-lg text-sm">
                          Task Completed - Members Locked
                        </span>
                      )}
                    </div>
                  )}
                </div>
              ))
            ) : (
              <p className="text-white/90">No tasks created yet.</p>
            )}
          </div>
        </div>
        {showAddTaskModal && isManager && (
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

        {/* Member Management Modal */}
        {showMemberModal && isManager && (
          <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50">
            <div className="bg-white/10 backdrop-blur-lg rounded-xl p-6 max-w-md w-full shadow-2xl border border-white/20">
              <div className="flex justify-between items-start mb-4">
                <h3 className="text-xl font-bold text-white">
                  {memberAction === 'add' ? 'Add Member to Task' : 'Remove Member from Task'}
                </h3>
                <button 
                  onClick={() => setShowMemberModal(false)}
                  className="text-white/80 hover:text-white"
                >
                  ✕
                </button>
              </div>

              {error && (
                <div className="mb-4 p-3 bg-red-500/20 backdrop-blur-lg border border-red-500/50 rounded-lg text-white text-sm">
                  {error}
                </div>
              )}

              <div className="space-y-4">
                <div>
                  <label className="block text-white mb-2">
                    Select Member to {memberAction === 'add' ? 'Add' : 'Remove'}:
                  </label>
                  <select
                    value={selectedMember}
                    onChange={(e) => setSelectedMember(e.target.value)}
                    className="w-full px-3 py-2 bg-white/10 border border-white/20 rounded-lg text-white focus:outline-none focus:ring-2 focus:ring-emerald-500 [&>option]:text-gray-900 [&>option]:bg-white"
                  >
                    <option value="">-- Select a member --</option>
                    {getAvailableMembers().map((member) => (
                      <option key={member} value={member}>
                        {member}
                      </option>
                    ))}
                  </select>
                </div>

                {memberAction === 'add' && (
                  <div className="p-3 bg-blue-500/10 border border-blue-400/20 rounded-lg">
                    <p className="text-blue-300 text-sm">
                      ℹ️ Only project members can be added to tasks.
                    </p>
                  </div>
                )}

                {memberAction === 'remove' && (
                  <div className="p-3 bg-yellow-500/10 border border-yellow-400/20 rounded-lg">
                    <p className="text-yellow-300 text-sm">
                      ⚠️ Cannot remove members from completed tasks.
                    </p>
                  </div>
                )}

                <div className="flex justify-end gap-3 pt-2">
                  <button
                    type="button"
                    onClick={() => setShowMemberModal(false)}
                    className="px-4 py-2 bg-white/20 text-white rounded-lg hover:bg-white/30 transition-colors"
                  >
                    Cancel
                  </button>
                  <button
                    onClick={submitMemberAction}
                    disabled={!selectedMember}
                    className={`px-4 py-2 rounded-lg transition-colors ${
                      memberAction === 'add' 
                        ? 'bg-blue-600 hover:bg-blue-700 text-white' 
                        : 'bg-red-600 hover:bg-red-700 text-white'
                    } ${!selectedMember ? 'opacity-50 cursor-not-allowed' : ''}`}
                  >
                    {memberAction === 'add' ? 'Add Member' : 'Remove Member'}
                  </button>
                </div>
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};

export default AddTasks; 