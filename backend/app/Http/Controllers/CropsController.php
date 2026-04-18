<?php

namespace App\Http\Controllers;

use App\Models\Crops;
use Illuminate\Http\Request;

class CropsController extends Controller
{
    public function index(Request $request) {
        $crops = Crops::where('user_id', $request->user()->id)
                     ->orWhere('is_predefined', true)
                     ->get();
        return response()->json(['success' => true, 'data' => $crops]);
    }

    public function store(Request $request) {
        $data = $request->validate([
            'name' => 'required|string',
            'water_need' => 'required|integer',
            'humidity_threshold' => 'required|integer',
            'min_water_level' => 'required|integer',
            'irrigations_per_day' => 'required|integer',
            'description' => 'nullable|string',
        ]);

        $data['user_id'] = $request->user()->id;
        $crop = Crops::create($data);

        return response()->json(['success' => true, 'data' => $crop], 201);
    }
    public function destroy(Request $request, $id) {
        $crop = Crops::where('user_id', $request->user()->id)->findOrFail($id);
        $crop->delete();
        return response()->json(['success' => true, 'message' => 'Culture supprimée']);
    }
}