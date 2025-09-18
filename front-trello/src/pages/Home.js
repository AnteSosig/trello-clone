import React, { useState, useEffect, useCallback } from 'react';
import { useNavigate } from 'react-router-dom';
import debounce from 'lodash/debounce';
import { projectApi, userApi } from '../utils/axios';
import { useAuth } from '../contexts/AuthContext';
import { ManagerOnly, RoleSwitch } from '../components/RoleBasedRender';

const Home = () => {
  const [isGridView, setIsGridView] = useState(true);
  const [cards, setCards] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [selectedCard, setSelectedCard] = useState(null);
  const [showNewProjectModal, setShowNewProjectModal] = useState(false);
  const { user, isManager } = useAuth();

  const fetchProjects = useCallback(async () => {
    try {
      setLoading(true);
      const response = await projectApi.get('/projects');
      const data = response.data;
      
      const transformedData = data.map(project => ({
        ...project,
        min_members: Number(project.min_members?.$numberInt || project.min_members),
        max_members: Number(project.max_members?.$numberInt || project.max_members),
        current_member_count: Number(project.current_member_count?.$numberInt || project.current_member_count),
      }));
      
      const filteredData = transformedData.filter(project => {
        if (user.role === 'MANAGER') {
          return project.moderator === user.id;
        } else if (user.role === 'USER') {
          return project.members.includes(user.id);
        }
        return false;
      });
      
      console.log('All projects:', data);
      console.log('Filtered projects:', filteredData);
      console.log('Current user ID:', user.id);
      setCards(filteredData);
    } catch (err) {
      console.error('Error fetching projects:', err);
      setError(err.message);
    } finally {
      setLoading(false);
    }
  }, [user]);

  useEffect(() => {
    if (user) {
      fetchProjects();
    }
  }, [user, fetchProjects]);

  if (loading) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 flex items-center justify-center">
        <div className="text-white text-xl">Loading projects...</div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700 flex items-center justify-center">
        <div className="text-white text-xl">Error: {error}</div>
      </div>
    );
  }

  const DetailModal = ({ card, onClose }) => {
    const navigate = useNavigate();
    const [showDeleteConfirm, setShowDeleteConfirm] = useState(false);
    const [deleting, setDeleting] = useState(false);

    const handleClose = () => {
      onClose();
      fetchProjects();
    };

    const handleDelete = async () => {
      try {
        setDeleting(true);
        await projectApi.delete(`/deleteproject/${projectId}`);
        alert('Project deleted successfully!');
        handleClose();
      } catch (err) {
        console.error('Delete error:', err);
        if (err.response?.status === 400) {
          alert('Cannot delete project - it has unfinished tasks');
        } else if (err.response?.status === 403) {
          alert('Access denied - only managers can delete projects');
        } else {
          alert('Failed to delete project: ' + (err.response?.data?.message || err.message));
        }
      } finally {
        setDeleting(false);
        setShowDeleteConfirm(false);
      }
    };

    if (!card) return null;

    const projectId = card._id?.$oid || card._id;

    return (
      <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50">
        <div className="bg-white/10 backdrop-blur-lg rounded-xl p-8 max-w-2xl w-full shadow-2xl border border-white/20">
            <div className="flex justify-between items-start mb-6">
              <h2 className="text-3xl font-bold text-white">{card.project}</h2>
              <div className="flex gap-4">
                <ManagerOnly>
                  <button 
                    onClick={() => {
                      handleClose();
                      navigate(`/edit-project/${projectId}`);
                    }}
                    className="text-white/80 hover:text-white px-4 py-2 bg-emerald-600/20 rounded-lg"
                  >
                    Edit Members
                  </button>
                </ManagerOnly>
                <button 
                  onClick={() => {
                    handleClose();
                    navigate(`/add-tasks/${projectId}`);
                  }}
                  className="text-white/80 hover:text-white px-4 py-2 bg-emerald-600/20 rounded-lg"
                >
                  <RoleSwitch 
                    managerContent="Add Tasks"
                    userContent="Tasks"
                    fallback="Tasks"
                  />
                </button>
              <button 
                onClick={handleClose}
                className="text-white/80 hover:text-white"
              >
                ✕
              </button>
            </div>
          </div>

          <div className="space-y-4">
            <div className="text-white/90">
              <p className="font-semibold">Moderator:</p>
              <p>{card.moderator}</p>
            </div>

            <div className="text-white/90">
              <p className="font-semibold">Description:</p>
              <p>{card.description}</p>
            </div>

            <div className="text-white/90">
              <p className="font-semibold">Estimated Completion:</p>
              <p>{card.estimated_completion_date}</p>
            </div>

            <div className="text-white/90">
              <p className="font-semibold">Member Capacity:</p>
              <p>{card.current_member_count} / {card.max_members} members (Minimum required: {card.min_members})</p>
            </div>

            <div className="text-white/90">
              <p className="font-semibold">Members:</p>
              <ul className="list-disc list-inside">
                {card.members.map((member, index) => (
                  <li key={index}>{member}</li>
                ))}
              </ul>
            </div>

            <div className="text-white/90">
              <p className="font-semibold">Status:</p>
              <span className={`inline-block px-3 py-1 rounded-full text-sm font-medium ${
                card.status === 1 
                  ? 'bg-green-500/20 text-green-300 border border-green-400/30' 
                  : 'bg-yellow-500/20 text-yellow-300 border border-yellow-400/30'
              }`}>
                {card.status === 1 ? 'Completed' : 'Active'}
              </span>
            </div>
          </div>

          {/* Delete Button - Bottom Right */}
          {isManager() && (
            <div className="flex justify-end mt-6">
              {!showDeleteConfirm ? (
                <button
                  onClick={() => setShowDeleteConfirm(true)}
                  className="px-4 py-2 bg-red-600/20 text-red-300 border border-red-400/30 rounded-lg hover:bg-red-600/30 transition-colors"
                >
                  Delete Project
                </button>
              ) : (
                <div className="flex gap-2">
                  <button
                    onClick={() => setShowDeleteConfirm(false)}
                    className="px-4 py-2 bg-gray-600/20 text-gray-300 border border-gray-400/30 rounded-lg hover:bg-gray-600/30 transition-colors"
                  >
                    Cancel
                  </button>
                  <button
                    onClick={handleDelete}
                    disabled={deleting}
                    className="px-4 py-2 bg-red-600 text-white rounded-lg hover:bg-red-700 transition-colors disabled:opacity-50"
                  >
                    {deleting ? 'Deleting...' : 'Confirm Delete'}
                  </button>
                </div>
              )}
            </div>
          )}
        </div>
      </div>
    );
  };

  const NewProjectModal = ({ onClose }) => {
    const [formData, setFormData] = useState({
      project: '',
      estimated_completion_date: '',
      min_members: 1,
      max_members: 2,
      members: ['']
    });
    const [searchResults, setSearchResults] = useState({});
    const [isSearching, setIsSearching] = useState({});

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

    const handleSubmit = async (e) => {
      e.preventDefault();
      
      try {
        const response = await projectApi.post('/newproject', {
          ...formData,
          moderator: user.id,
          current_member_count: formData.members.filter(m => m).length
        });

        await fetchProjects();
        onClose();
      } catch (err) {
        console.error('Error creating project:', err);
        alert('Failed to create project');
      }
    };

    const addMemberField = () => {
      if (formData.members.length < formData.max_members) {
        setFormData(prev => ({
          ...prev,
          members: [...prev.members, '']
        }));
      }
    };

    const removeMemberField = (index) => {
      setFormData(prev => ({
        ...prev,
        members: prev.members.filter((_, i) => i !== index)
      }));
      setSearchResults(prev => {
        const newResults = { ...prev };
        delete newResults[index];
        return newResults;
      });
    };

    const updateMember = (index, value) => {
      setFormData(prev => ({
        ...prev,
        members: prev.members.map((m, i) => i === index ? value : m)
      }));
      
      if (value.length >= 4 && !searchResults[index]?.some(user => user.username === value)) {
        debouncedSearch(value, index);
      } else {
        setSearchResults(prev => ({ ...prev, [index]: [] }));
      }
    };

    const selectUser = (index, username) => {
      setSearchResults(prev => ({ ...prev, [index]: [] }));
      
      setTimeout(() => {
        setFormData(prev => ({
          ...prev,
          members: prev.members.map((m, i) => i === index ? username : m)
        }));
      }, 100);
    };

    const handleInputChange = (e) => {
      const { name, value } = e.target;
      setFormData(prev => ({
        ...prev,
        [name]: value
      }));
    };

    return (
      <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50">
        <div className="bg-white/10 backdrop-blur-lg rounded-xl p-8 max-w-2xl w-full shadow-2xl border border-white/20">
          <div className="flex justify-between items-start mb-6">
            <h2 className="text-3xl font-bold text-white">Create New Project</h2>
            <button onClick={onClose} className="text-white/80 hover:text-white">✕</button>
          </div>

          <form onSubmit={handleSubmit} className="space-y-6">
            <div>
              <label className="block text-white mb-2">Project Name</label>
              <input
                type="text"
                name="project"
                required
                className="w-full px-4 py-2 rounded-lg bg-white/10 border border-white/20 text-white"
                value={formData.project}
                onChange={handleInputChange}
              />
            </div>

            <div>
              <label className="block text-white mb-2">Estimated Completion Date</label>
              <input
                type="date"
                name="estimated_completion_date"
                required
                className="w-full px-4 py-2 rounded-lg bg-white/10 border border-white/20 text-white"
                value={formData.estimated_completion_date}
                onChange={handleInputChange}
              />
            </div>

            <div className="flex gap-4">
              <div className="flex-1">
                <label className="block text-white mb-2">Min Members</label>
                <input
                  type="number"
                  name="min_members"
                  required
                  min="1"
                  className="w-full px-4 py-2 rounded-lg bg-white/10 border border-white/20 text-white"
                  value={formData.min_members}
                  onChange={e => handleInputChange({ target: { name: 'min_members', value: parseInt(e.target.value) } })}
                />
              </div>
              <div className="flex-1">
                <label className="block text-white mb-2">Max Members</label>
                <input
                  type="number"
                  name="max_members"
                  required
                  min={formData.min_members}
                  className="w-full px-4 py-2 rounded-lg bg-white/10 border border-white/20 text-white"
                  value={formData.max_members}
                  onChange={e => handleInputChange({ target: { name: 'max_members', value: parseInt(e.target.value) } })}
                />
              </div>
            </div>

            <div>
              <label className="block text-white mb-2">Members</label>
              {formData.members.map((member, index) => (
                <div key={index} className="relative mb-2">
                  <div className="flex gap-2">
                    <div className="flex-1 relative">
                      <input
                        type="text"
                        className="w-full px-4 py-2 rounded-lg bg-white/10 border border-white/20 text-white"
                        value={member}
                        onChange={(e) => updateMember(index, e.target.value)}
                        placeholder="Member name"
                      />
                      {isSearching[index] && (
                        <div className="absolute right-3 top-1/2 transform -translate-y-1/2">
                          <div className="animate-spin rounded-full h-4 w-4 border-b-2 border-white"></div>
                        </div>
                      )}
                    </div>
                    <button
                      type="button"
                      onClick={() => removeMemberField(index)}
                      className="px-4 py-2 bg-red-500/20 text-white rounded-lg hover:bg-red-500/30"
                    >
                      Remove
                    </button>
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

              {formData.members.length < formData.max_members && (
                <button
                  type="button"
                  onClick={addMemberField}
                  className="mt-2 px-4 py-2 bg-emerald-600/20 text-white rounded-lg hover:bg-emerald-600/30"
                >
                  Add Member
                </button>
              )}
            </div>

            <button
              type="submit"
              className="w-full px-6 py-3 bg-emerald-600 text-white rounded-lg hover:bg-emerald-700 transition-colors"
            >
              Create Project
            </button>
          </form>
        </div>
      </div>
    );
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-green-900 via-emerald-800 to-teal-700">
      {selectedCard && (
        <DetailModal 
          card={selectedCard} 
          onClose={() => setSelectedCard(null)} 
        />
      )}
      
      {showNewProjectModal && (
        <NewProjectModal onClose={() => setShowNewProjectModal(false)} />
      )}

      <div className="container mx-auto px-4 py-12">
        {/* Header Section */}
        <div className="bg-white/10 backdrop-blur-lg rounded-xl p-8 mb-8 max-w-4xl mx-auto text-center shadow-2xl">
          <h1 className="text-4xl md:text-5xl font-bold text-white mb-4">
            Dobrodosli na Trello Clone
          </h1>
          <p className="text-xl text-white/90 leading-relaxed mb-6">
            
          </p>
          
          {/* View Toggle and New Project Buttons */}
          <div className="flex justify-center gap-4">
            <button
              onClick={() => setIsGridView(!isGridView)}
              className="px-6 py-2 bg-white/20 text-white rounded-lg font-medium 
                hover:bg-white/30 transition-colors duration-300"
            >
              {isGridView ? 'Switch to List View' : 'Switch to Grid View'}
            </button>
            
            <ManagerOnly>
              <button
                onClick={() => setShowNewProjectModal(true)}
                className="px-6 py-2 bg-emerald-600 text-white rounded-lg font-medium 
                  hover:bg-emerald-700 transition-colors duration-300"
              >
                New Project
              </button>
            </ManagerOnly>
          </div>
        </div>

        {/* Cards Container */}
        <div className={`grid gap-6 ${isGridView ? 'grid-cols-1 md:grid-cols-2 lg:grid-cols-3' : 'grid-cols-1'}`}>
          {cards.map((card, index) => (
            <div
              key={index}
              className={`bg-white/10 backdrop-blur-lg rounded-xl p-6 shadow-xl 
                hover:transform hover:scale-105 transition-all duration-300
                border border-white/20 group relative
                ${isGridView ? 'h-48' : 'h-32'}`}
            >
              <div className="h-full flex flex-col">
                {isGridView ? (
                  <>
                    <div>
                      <h3 className="text-xl font-semibold text-white mb-2">
                        {card.project}
                      </h3>
                      <p className="text-white/80">
                        {card.description}
                      </p>
                      <p className="text-white/80">
                        Members: {card.current_member_count}/{card.max_members}
                      </p>
                    </div>
                    <div className="absolute bottom-6 left-6 right-6 flex justify-between items-center opacity-0 group-hover:opacity-100 transition-opacity">
                      <div className="flex items-center">
                        <span className={`px-3 py-1 rounded-full text-xs font-medium ${
                          card.status === 1 
                            ? 'bg-green-500/20 text-green-300 border border-green-400/30' 
                            : 'bg-yellow-500/20 text-yellow-300 border border-yellow-400/30'
                        }`}>
                          {card.status === 1 ? 'Completed' : 'Active'}
                        </span>
                      </div>
                      <button 
                        onClick={() => setSelectedCard(card)}
                        className="px-4 py-1 bg-white/20 text-white rounded-full text-sm
                          hover:bg-white/30 transition-colors duration-300 whitespace-nowrap"
                      >
                        View Details
                      </button>
                    </div>
                  </>
                ) : (
                  <div className="flex justify-between items-center w-full h-full">
                    <div className="flex items-center gap-4">
                      <div>
                        <h3 className="text-xl font-semibold text-white mb-2">
                          {card.project}
                        </h3>
                        <p className="text-white/80">
                          Members: {card.current_member_count}/{card.max_members}
                        </p>
                      </div>
                      <span className={`px-3 py-1 rounded-full text-xs font-medium ${
                        card.status === 1 
                          ? 'bg-green-500/20 text-green-300 border border-green-400/30' 
                          : 'bg-yellow-500/20 text-yellow-300 border border-yellow-400/30'
                      }`}>
                        {card.status === 1 ? 'Completed' : 'Active'}
                      </span>
                    </div>
                    <div className="opacity-0 group-hover:opacity-100 transition-opacity ml-4">
                      <button 
                        onClick={() => setSelectedCard(card)}
                        className="px-4 py-1 bg-white/20 text-white rounded-full text-sm
                          hover:bg-white/30 transition-colors duration-300 whitespace-nowrap"
                      >
                        View Details
                      </button>
                    </div>
                  </div>
                )}
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};

export default Home;