import React, { useState, useEffect, useCallback } from 'react';
import { useNavigate } from 'react-router-dom';
import debounce from 'lodash/debounce';
import { jwtDecode } from "jwt-decode";
import { getAuthHeader } from '../utils/cookies';

const Home = () => {
  const [isGridView, setIsGridView] = useState(true);
  const [cards, setCards] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [selectedCard, setSelectedCard] = useState(null);
  const [showNewProjectModal, setShowNewProjectModal] = useState(false);
  const [userRole, setUserRole] = useState(null);
  const [userId, setUserId] = useState(null);

  const fetchProjects = useCallback(async () => {
    try {
      setLoading(true);
      const response = await fetch('https://localhost:8443/project/projects', {
        headers: {
          ...getAuthHeader()
        }
      });
      if (!response.ok) {
        throw new Error('Failed to fetch projects');
      }
      const data = await response.json();
      
      const transformedData = data.map(project => ({
        ...project,
        min_members: Number(project.min_members?.$numberInt || project.min_members),
        max_members: Number(project.max_members?.$numberInt || project.max_members),
        current_member_count: Number(project.current_member_count?.$numberInt || project.current_member_count),
      }));
      
      const filteredData = transformedData.filter(project => {
        if (userRole === 'MANAGER') {
          return project.moderator === userId;
        } else if (userRole === 'USER') {
          return project.members.includes(userId);
        }
        return false;
      });
      
      console.log('All projects:', data);
      console.log('Filtered projects:', filteredData);
      console.log('Current user ID:', userId);
      setCards(filteredData);
    } catch (err) {
      console.error('Error fetching projects:', err);
      setError(err.message);
    } finally {
      setLoading(false);
    }
  }, [userId, userRole]);

  useEffect(() => {
    try {
      const tokenCookie = document.cookie
        .split('; ')
        .find(row => row.startsWith('token='));
      
      if (!tokenCookie) {
        console.error('No token found in cookies');
        setError('Authentication required');
        setLoading(false);
        return;
      }

      const token = tokenCookie.split('=')[1];
      const decodedToken = jwtDecode(token);
      console.log('Decoded token:', decodedToken);
      setUserRole(decodedToken.aud);
      setUserId(decodedToken.sub);
      console.log('User ID set to:', decodedToken.sub);

      fetchProjects();
    } catch (err) {
      console.error('Error processing token:', err);
      setError('Authentication error');
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    if (userId && userRole) {
      fetchProjects();
    }
  }, [userId, userRole, fetchProjects]);

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

    const handleClose = () => {
      onClose();
      fetchProjects();
    };

    if (!card) return null;

    const projectId = card._id?.$oid || card._id;

    return (
      <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50">
        <div className="bg-white/10 backdrop-blur-lg rounded-xl p-8 max-w-2xl w-full shadow-2xl border border-white/20">
          <div className="flex justify-between items-start mb-6">
            <h2 className="text-3xl font-bold text-white">{card.project}</h2>
            <div className="flex gap-4">
              <button 
                onClick={() => {
                  handleClose();
                  navigate(`/edit-project/${projectId}`);
                }}
                className="text-white/80 hover:text-white px-4 py-2 bg-emerald-600/20 rounded-lg"
              >
                Edit Members
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
          </div>
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
            const response = await fetch(`https://localhost:8443/user/finduser?name=${searchTerm}`);
            const data = await response.json();
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
        const response = await fetch('https://localhost:8443/project/newproject', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            ...getAuthHeader()
          },
          body: JSON.stringify({
            ...formData,
            moderator: userId,
            current_member_count: formData.members.filter(m => m).length
          }),
        });

        if (!response.ok) {
          throw new Error('Failed to create project');
        }

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
            Svi projekti za vase prijatelje i gejmere na jednom mestu
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
            
            {userRole === 'MANAGER' && (
              <button
                onClick={() => setShowNewProjectModal(true)}
                className="px-6 py-2 bg-emerald-600 text-white rounded-lg font-medium 
                  hover:bg-emerald-700 transition-colors duration-300"
              >
                New Project
              </button>
            )}
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
                    <div className="absolute bottom-6 right-6 opacity-0 group-hover:opacity-100 transition-opacity">
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
                  <div className="flex justify-between items-start w-full">
                    <div>
                      <h3 className="text-xl font-semibold text-white mb-2">
                        {card.project}
                      </h3>
                      <p className="text-white/80">
                        Members: {card.current_member_count}/{card.max_members}
                      </p>
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