<?php

namespace App\Http\Controllers;

use App\Models\IrrigationTime;
use Illuminate\Http\Request;

class IrrigationTimeController extends Controller
{
    public function store(Request $request) {
        $request->validate([
            'crop_id' => 'required|exists:crops,id',
            'time' => 'required',
            'order_index' => 'required|integer'
        ]);

        $time = IrrigationTime::create($request->all());
        return response()->json(['success' => true, 'data' => $time]);
    }

    public function destroy($id) {
        IrrigationTime::destroy($id);
        return response()->json(['success' => true, 'message' => 'Supprimé']);
    }
}