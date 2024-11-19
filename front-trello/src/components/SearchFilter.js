import React, { useState, useEffect } from 'react';
import axios from 'axios';

const FilterComponent = () => {
  const [lokacija, setLokacija] = useState('');
  const [povrsina, setPovrsina] = useState('');
  const [cena, setCena] = useState('');
  const [prodaja, setProdaja] = useState('');
  const [tip, setTip] = useState('');
  const [filteredProperties, setFilteredProperties] = useState([]);

  useEffect(() => {
    handleFilter();
  }, []);

  const handleFilter = async () => {
    try {
      const response = await axios.get('http://localhost:8080/api/nekretnine/pretraga', {
        params: { lokacija, povrsina, cena, prodaja, tip },
        headers: { authorization: 'token' },
      });
      setFilteredProperties(response.data);
    } catch (error) {
      console.error('Error fetching data:', error);
    }
  };

  const handleLike = async (propertyId) => {
    try {
      const token = localStorage.getItem('token');
      const response = await axios.post(
        'http://localhost:8080/api/nekretnine/like',
        { nekretninaId: propertyId, like: true },
        { headers: { authorization: token } }
      );
      if (response.status === 200) {
        handleFilter();
      }
    } catch (error) {
      console.error('Error liking property:', error);
    }
  };

  return (
    <div className="container mx-auto px-4 py-8">
      {/* Search Filters */}
      <div className="bg-white rounded-xl shadow-lg p-6 mb-8">
        <div className="grid grid-cols-1 md:grid-cols-3 lg:grid-cols-6 gap-4">
          <input
            type="text"
            placeholder="Lokacija"
            value={lokacija}
            onChange={(e) => setLokacija(e.target.value)}
            className="w-full px-4 py-2 rounded-lg border border-gray-200 focus:border-emerald-500 
                     focus:ring-2 focus:ring-emerald-200 outline-none transition-all duration-200"
          />
          <input
            type="text"
            placeholder="Površina"
            value={povrsina}
            onChange={(e) => setPovrsina(e.target.value)}
            className="w-full px-4 py-2 rounded-lg border border-gray-200 focus:border-emerald-500 
                     focus:ring-2 focus:ring-emerald-200 outline-none transition-all duration-200"
          />
          <input
            type="text"
            placeholder="Cena"
            value={cena}
            onChange={(e) => setCena(e.target.value)}
            className="w-full px-4 py-2 rounded-lg border border-gray-200 focus:border-emerald-500 
                     focus:ring-2 focus:ring-emerald-200 outline-none transition-all duration-200"
          />
          <input
            type="text"
            placeholder="Prodaja/Izdavanje"
            value={prodaja}
            onChange={(e) => setProdaja(e.target.value)}
            className="w-full px-4 py-2 rounded-lg border border-gray-200 focus:border-emerald-500 
                     focus:ring-2 focus:ring-emerald-200 outline-none transition-all duration-200"
          />
          <input
            type="text"
            placeholder="Tip nekretnine"
            value={tip}
            onChange={(e) => setTip(e.target.value)}
            className="w-full px-4 py-2 rounded-lg border border-gray-200 focus:border-emerald-500 
                     focus:ring-2 focus:ring-emerald-200 outline-none transition-all duration-200"
          />
          <button
            onClick={handleFilter}
            className="w-full px-4 py-2 bg-emerald-600 text-white rounded-lg hover:bg-emerald-700 
                     transition-colors duration-200 font-medium"
          >
            Pretraži
          </button>
        </div>
      </div>

      {/* Results Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        {filteredProperties.map((property) => (
          <div
            key={property.id}
            className="bg-white rounded-xl shadow-lg overflow-hidden hover:shadow-xl transition-shadow duration-300"
          >
            <div className="p-6">
              <div className="flex justify-between items-start mb-4">
                <h3 className="text-xl font-semibold text-gray-800">
                  {property.lokacija}
                </h3>
                <span className="px-3 py-1 bg-emerald-100 text-emerald-800 rounded-full text-sm font-medium">
                  {property.tip}
                </span>
              </div>
              
              <div className="space-y-2 mb-4">
                <p className="text-gray-600">Površina: {property.povrsina} m²</p>
                <p className="text-gray-600">Cena: {property.cena} €</p>
                <p className="text-gray-600">{property.prodajaIzdaja}</p>
              </div>

              <div className="flex justify-between items-center pt-4 border-t border-gray-100">
                <button
                  onClick={() => handleLike(property.id)}
                  className="flex items-center space-x-2 text-gray-600 hover:text-emerald-600 transition-colors"
                >
                  <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" 
                          d="M4.318 6.318a4.5 4.5 0 000 6.364L12 20.364l7.682-7.682a4.5 4.5 0 00-6.364-6.364L12 7.636l-1.318-1.318a4.5 4.5 0 00-6.364 0z" />
                  </svg>
                  <span>{property.likes}</span>
                </button>
                
                <button
                  onClick={() => window.location.href = `/nekretnina/${property.id}`}
                  className="px-4 py-2 bg-emerald-600 text-white rounded-lg hover:bg-emerald-700 
                           transition-colors duration-200 text-sm font-medium"
                >
                  Detaljnije
                </button>
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
};

export default FilterComponent;
