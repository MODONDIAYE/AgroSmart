<?php

namespace App\Http\Controllers;

use App\Http\Controllers;
use Illuminate\Http\Request;
use App\Models\User;
use Illuminate\Support\Facades\Hash;

class AuthController extends Controller
{
    public function login(Request $request)
    {
        // Validation
        $request->validate([
            'email' => 'required|email',
            'password' => 'required'
        ]);

        // Trouver l'utilisateur
        $user = User::where('email', $request->email)->first();

        if (!$user || !Hash::check($request->password, $user->password)) {
            return response()->json([
                'success' => false,
                'message' => 'Invalid email or password'
            ], 401);
        }

        // Générer token (Sanctum)
        $token = $user->createToken('auth_token')->plainTextToken;

        return response()->json([
            'success' => true,
            'message' => 'Login successful',
            'data' => [
                'user' => $user,
                'token' => $token
            ]
        ], 200);
    }

    public function register(Request $request)
{
    // 1. Validation des données entrants
    $request->validate([
        'username' => 'required|string|min:3',
        'email' => 'required|string|email|max:20|unique:users',
        'password' => 'required|string|min:6', // 'confirmed' cherche un champ password_confirmation
        'phone' => 'nullable|string|max:20',
    ]);

    // 2. Création de l'utilisateur
    $user = User::create([
        'username' => $request->username,
        'email' => $request->email,
        // Toujours hacher le mot de passe avant stockage
        'password' => Hash::make($request->password), 
        'phone' => $request->phone,
    ]);

    // 3. Génération du token (Sanctum)
    $token = $user->createToken('auth_token')->plainTextToken;

    // 4. Réponse JSON
    return response()->json([
        'success' => true,
        'message' => 'User registered successfully',
        'data' => [
            'user' => $user,
            'token' => $token,
        
        ]
    ], 201);
}
}




