import React, { useEffect, useState, useRef } from "react";
import axios from "axios";

const AccountConfirm = () => {
  const [status, setStatus] = useState(null); // null for loading, true for success, false for error
  const activationAttempted = useRef(false);

  useEffect(() => {
    if (activationAttempted.current) return;
    
    const queryParams = new URLSearchParams(window.location.search);
    const link = queryParams.get("link");

    if (link) {
      activationAttempted.current = true;
      axios
        .get(`http://localhost:8080/activate?link=${link}`)
        .then((response) => {
          if (response.status === 200) {
            setStatus(true); // Success
          } else {
            setStatus(false); // Failed
          }
        })
        .catch((error) => {
          setStatus(false); // Failed
        });
    } else {
      setStatus(false); // No link provided
    }
  }, []);

  return (
    <div className="flex items-center justify-center h-screen bg-gray-100">
      <div className="text-center">
        {status === null ? (
          <p className="text-lg text-gray-600">Activating your account...</p>
        ) : status === true ? (
          <p className="text-2xl text-green-500 font-bold">Account Activated!</p>
        ) : (
          <p className="text-2xl text-red-500 font-bold">Failed to Activate Account</p>
        )}
      </div>
    </div>
  );
};

export default AccountConfirm;