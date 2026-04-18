<?php

namespace App\Http\Controllers;

use App\Models\Notification;
use Illuminate\Http\Request;

class NotificationController extends Controller
{
    public function index(Request $request)
    {
        $notifications = Notification::where('user_id', $request->user()->id)
                                     ->orderBy('created_at', 'desc')
                                     ->get();

        return response()->json(['success' => true, 'data' => $notifications]);
    }

    public function markAsRead(Request $request, $id)
    {
        Notification::where('id', $id)
                    ->where('user_id', $request->user()->id)
                    ->update(['is_read' => true]);

        return response()->json(['success' => true]);
    }

    public function markAllRead(Request $request)
    {
        Notification::where('user_id', $request->user()->id)
                    ->update(['is_read' => true]);

        return response()->json(['success' => true]);
    }
}
