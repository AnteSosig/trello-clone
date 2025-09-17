import React, { useState, useEffect, useCallback } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import debounce from 'lodash/debounce';
import { projectApi, userApi } from '../utils/axios';
import { useAuth } from '../contexts/AuthContext';

const EditProject = () => {
  const { id } = useParams();
  const navigate = useNavigate();
  const [project, setProject] = useState(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [searchResults, setSearchResults] = useState({});
  const [isSearching, setIsSearching] = useState({});
  const { user } = useAuth();

  // Debounced search function
  const debouncedSearch = useCallback(
    debounce(async (searchTerm, index) => {
      if (searchTerm.length >= 4) {
        setIsSearching(prev => ({ ...prev, [index]: true }));
        try {
          const response = await userApi.get(`/finduser?name=${searchTerm}`);
          const data = response.data;
          setSearchResults(prev => ({ ...prev, [index]: data }));
        } catch (error) {
          console.error('Error searching users:', error);
        } finally {
          setIsSearching(prev => ({ ...prev, [index]: false }));
        }
      } else {
        setSearchResults(prev => ({ ...prev, [index]: [] }));
      }
    }, 300),
    []
  );

  useEffect(() => {
    const fetchProject = async () => {
      try {
        if (!user) {
          setError('Authentication required');
          navigate('/login');
          return;
        }
        
        const response = await projectApi.get(`/projects/${id}`);
        const data = response.data;
        
        const transformedData = {
          ...data,
          _id: data._id?.$oid || data._id,
          min_members: Number(data.min_members?.$numberInt || data.min_members),
          max_members: Number(data.max_members?.$numberInt || data.max_members),
          current_member_count: Number(data.current_member_count?.$numberInt || data.current_member_count),
        };
        
        setProject(transformedData);
      } catch (err) {
        console.error('Error details:', err);
        if (err.response?.status === 401) {
          setError('Authentication failed - please login again');
          navigate('/login');
        } else if (err.response?.status === 403) {
          setError('Access denied - insufficient permissions');
        } else {
          setError(err.message || 'Failed to fetch project');
        }
      } finally {
        setLoading(false);
      }
    };

    if (user) {
      fetchProject();
    }
  }, [id, user, navigate]);

  const handleMemberUpdate = async (newMembers) => {
    try {
      console.log('Sending update request with members:', newMembers);
      
      const response = await projectApi.patch(`/updateproject/${id}`, {
        members: newMembers
      });
      
      setProject(prev => ({
        ...prev,
        members: newMembers,
        current_member_count: newMembers.length
      }));
    } catch (err) {
      console.error('Update error:', err);
      if (err.response?.status === 401) {
        setError('Authentication failed - please login again');
        navigate('/login');
      } else if (err.response?.status === 403) {
        setError('Access denied - insufficient permissions');
      } else {
        setError(err.message || 'Failed to update members');
      }
    }
  };

  const addMember = () => {
    if (project.members.length < project.max_members) {
      const newMembers = [...project.members, ''];
      setProject(prev => ({
        ...prev,
        members: newMembers
      }));
    }
  };

  const removeMember = (index) => {
    if (project.members.length > project.min_members) {
      const newMembers = project.members.filter((_, i) => i !== index);
      handleMemberUpdate(newMembers);
    }
  };

  const updateMember = (index, value) => {
    const newMembers = project.members.map((m, i) => i === index ? value : m);
    setProject(prev => ({
      ...prev,
      members: newMembers
    }));
    
    if (value.length >= 4 && !searchResults[index]?.some(user => user.username === value)) {
      debouncedSearch(value, index);
    } else {
      setSearchResults(prev => ({ ...prev, [index]: [] }));
    }
  };

  const selectUser = (index, username) => {
    setSearchResults(prev => ({ ...prev, [index]: [] }));
    const newMembers = project.members.map((m, i) => i === index ? username : m);
    setProject(prev => ({
      ...prev,
      members: newMembers
    }));
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    await handleMemberUpdate(project.members);
    navigate('/');
  };

  if (loading) return <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 flex items-center justify-center">
    <div className="text-white text-xl">Loading project...</div>
  </div>;

  if (error) return <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 flex items-center justify-center">
    <div className="text-white text-xl">Error: {error}</div>
  </div>;

  if (!project) return null;

  return (
    <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 p-8">
      <div className="max-w-2xl mx-auto bg-white/10 backdrop-blur-lg rounded-xl p-8 shadow-2xl border border-white/20">
        <div className="flex justify-between items-center mb-8">
          <h1 className="text-3xl font-bold text-white">Edit Project: {project.project}</h1>
          <button
            onClick={() => navigate('/')}
            className="text-white/80 hover:text-white"
          >
            ← Back
          </button>
        </div>

        <div className="space-y-6">
          <div className="text-white/90">
            <p className="font-semibold">Moderator:</p>
            <p>{project.moderator}</p>
          </div>

          <div className="text-white/90">
            <p className="font-semibold">Member Capacity:</p>
            <p>{project.current_member_count} / {project.max_members} members (Minimum: {project.min_members})</p>
          </div>

          <form onSubmit={handleSubmit} className="space-y-6">
            <div>
              <label className="block text-white mb-4 font-semibold">Members:</label>
              {project.members.map((member, index) => (
                <div key={index} className="relative mb-2">
                  <div className="flex gap-2">
                    <div className="flex-1 relative">
                      <input
                        type="text"
                        value={member}
                        onChange={(e) => updateMember(index, e.target.value)}
                        className="w-full px-4 py-2 rounded-lg bg-white/10 border border-white/20 text-white"
                        placeholder="Member name"
                      />
                      {isSearching[index] && (
                        <div className="absolute right-3 top-1/2 transform -translate-y-1/2">
                          <div className="animate-spin rounded-full h-4 w-4 border-b-2 border-white"></div>
                        </div>
                      )}
                    </div>
                    {project.members.length > project.min_members && (
                      <button
                        type="button"
                        onClick={() => removeMember(index)}
                        className="px-4 py-2 bg-red-500/20 text-white rounded-lg hover:bg-red-500/30"
                      >
                        Remove
                      </button>
                    )}
                  </div>
                  
                  {searchResults[index]?.length > 0 && (
                    <div className="absolute z-50 w-full mt-1 bg-white/10 backdrop-blur-lg rounded-lg border border-white/20 max-h-48 overflow-y-auto">
                      {searchResults[index].map((user, userIndex) => (
                        <div
                          key={userIndex}
                          className="px-4 py-2 text-white hover:bg-white/20 cursor-pointer"
                          onClick={() => selectUser(index, user.username)}
                        >
                          {user.username} ({user.first_name} {user.last_name})
                        </div>
                      ))}
                    </div>
                  )}
                </div>
              ))}
              
              {project.members.length < project.max_members && (
                <button
                  type="button"
                  onClick={addMember}
                  className="mt-2 px-4 py-2 bg-white/20 text-white rounded-lg hover:bg-white/30"
                >
                  Add Member
                </button>
              )}
            </div>

            <div className="flex gap-4">
              <button
                type="submit"
                className="flex-1 px-6 py-3 bg-emerald-600 text-white rounded-lg hover:bg-emerald-700 transition-colors"
              >
                Save Changes
              </button>
              <button
                type="button"
                onClick={() => navigate('/')}
                className="px-6 py-3 bg-white/20 text-white rounded-lg hover:bg-white/30 transition-colors"
              >
                Cancel
              </button>
            </div>
          </form>
        </div>
      </div>
    </div>
  );
};

export default EditProject; 