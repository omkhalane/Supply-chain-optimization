import React, { useEffect, useRef, useState, useMemo } from "react";
import * as d3 from "d3";
import {
  AlertTriangle,
  Settings,
  Activity,
  ArrowRight,
  Truck,
  Factory,
  Warehouse,
  ShoppingCart,
  Plus,
  Minus,
  Maximize,
  Zap,
  ChevronDown,
} from "lucide-react";
import { motion, AnimatePresence } from "motion/react";
import { cn } from "./lib/utils";

// --- Types ---

interface InventoryLossEntry {
  item: string;
  node: string;
  quantity_lost: number;
  unit: string;
}

interface OutputData {
  inventory_losses: {
    affected_nodes: number;
    entries: InventoryLossEntry[];
    totals_by_unit: Record<string, number>;
  }[];
}

interface ImplementedOptimizationRoute {
  capacity: number;
  cost: number;
  links: string[];
  reactivated_node: string;
  route: string[];
  time: number;
}

interface SuggestionData {
  strategy: string;
  path: string[];
  cost: number;
  time: number;
  capacity: number;
  implemented_links: string[];
  implemented_optimization_routes: ImplementedOptimizationRoute[];
  unrecoverable_nodes: string[];
}

interface InventoryItem {
  item: string;
  quantity: number;
  unit: string;
}

interface Node extends d3.SimulationNodeDatum {
  id: string;
  type: "supplier" | "factory" | "warehouse" | "market";
  x: number;
  y: number;
  inventory?: InventoryItem[];
}

interface Road extends d3.SimulationLinkDatum<Node> {
  from: string;
  to: string;
  distance: number;
  capacity: number;
  tollTax: number;
  trafficDelay: number;
  source: Node;
  target: Node;
}

interface SimulationConfig {
  failureNode: string;
}

interface SupplyChainData {
  config: {
    petrolRate_INR: number;
    truckMileage_kmpl: number;
  };
  nodes: Node[];
  roads: Road[];
  simulation: SimulationConfig;
}

// --- Constants ---

const NODE_ICONS = {
  supplier: Truck,
  factory: Factory,
  warehouse: Warehouse,
  market: ShoppingCart,
};

const LAYER_ORDER = {
  supplier: 0,
  factory: 1,
  warehouse: 2,
  market: 3,
};

const OPTIMIZATION_MODES = [
  { id: "select", label: "Select", desc: "Choose an optimization strategy" },
  {
    id: "optimal",
    label: "Most Optimal",
    desc: "Safest - Balances Cost/Time/Capacity",
  },
  {
    id: "supply",
    label: "Max Supply",
    desc: "Prevents stockouts - Prioritizes High Capacity",
  },
  { id: "cost", label: "Min Cost", desc: "Budget preservation - Lowest INR" },
  { id: "time", label: "Min Time", desc: "Emergency rush - Fastest Delivery" },
];

// ~5cm visual target at 96dpi ≈ 189px
const MIN_NODE_GAP_PX = 190;
const MIN_LAYER_GAP_PX = 220;
const MIN_CONTENT_WIDTH = 1600;
const MIN_CONTENT_HEIGHT = 1600;
const LAYOUT_PADDING = 220;

// --- Components ---

export default function App() {
  const svgRef = useRef<SVGSVGElement>(null);
  const zoomRef = useRef<d3.ZoomBehavior<SVGSVGElement, unknown> | null>(null);
  const [data, setData] = useState<SupplyChainData | null>(null);
  const [outputData, setOutputData] = useState<OutputData | null>(null);
  const [hoveredNode, setHoveredNode] = useState<Node | null>(null);
  const [hoveredRoad, setHoveredRoad] = useState<Road | null>(null);
  const [impactedNodes, setImpactedNodes] = useState<Set<string>>(new Set());
  const [impactedRoads, setImpactedRoads] = useState<Set<string>>(new Set());
  const [transform, setTransform] = useState({ x: 0, y: 0, k: 1 });
  const [optimizationMode, setOptimizationMode] = useState("select");
  const [suggestionData, setSuggestionData] = useState<SuggestionData | null>(
    null,
  );

  // Load base input data from SupplyChain/input_data/input.json
  useEffect(() => {
    fetch("/input_data/input.json")
      .then((res) => res.json())
      .then((json) => setData(json))
      .catch((err) => {
        console.error("Error loading input data:", err);
        setData(null);
      });
  }, []);

  // Load latest inventory loss report from Backend/testdata/output.json (if present)
  useEffect(() => {
    fetch("/Backend/testdata/output.json")
      .then((res) => res.json())
      .then((json) => setOutputData(json))
      .catch(() => setOutputData(null));
  }, []);

  // Load suggestion data when mode changes
  useEffect(() => {
    if (optimizationMode === "select") {
      setSuggestionData(null);
      return;
    }

    const fileNameMap: Record<string, string> = {
      supply: "max_supply.json",
      cost: "min_cost.json",
      time: "min_time.json",
      optimal: "most_optimal.json",
    };

    const fileName = fileNameMap[optimizationMode];
    if (fileName) {
      fetch(`/Backend/suggestions/${fileName}`)
        .then((res) => res.json())
        .then((data) => setSuggestionData(data))
        .catch((err) => console.error("Error loading suggestion:", err));
    }
  }, [optimizationMode]);

  // Calculate hierarchical positions and impacts
  const processedData = useMemo(() => {
    if (!data) {
      return {
        nodes: [] as Node[],
        roads: [] as Road[],
        impactedNodes: new Set<string>(),
        impactedRoads: new Set<string>(),
        contentWidth: MIN_CONTENT_WIDTH,
        contentHeight: MIN_CONTENT_HEIGHT,
      };
    }

    // Clone nodes to avoid mutation
    const nodes: Node[] = data.nodes.map((n) => ({ ...n }));

    // Group nodes by layer
    const layers: Record<number, Node[]> = {};
    nodes.forEach((node) => {
      const layer = LAYER_ORDER[node.type as keyof typeof LAYER_ORDER];
      if (!layers[layer]) layers[layer] = [];
      layers[layer].push(node);
    });

    const layerIds = Object.keys(layers)
      .map((v) => Number(v))
      .sort((a, b) => a - b);

    const layerCount = Math.max(layerIds.length, 1);
    const maxNodesInLayer = Math.max(
      ...layerIds.map((layerId) => layers[layerId].length),
      1,
    );

    const contentWidth = Math.max(
      MIN_CONTENT_WIDTH,
      LAYOUT_PADDING * 2 + (layerCount - 1) * MIN_LAYER_GAP_PX,
    );

    const contentHeight = Math.max(
      MIN_CONTENT_HEIGHT,
      LAYOUT_PADDING * 2 + (maxNodesInLayer - 1) * MIN_NODE_GAP_PX,
    );

    // Assign fixed positions with strict spacing
    const xStep =
      layerCount > 1
        ? Math.max(
            MIN_LAYER_GAP_PX,
            (contentWidth - 2 * LAYOUT_PADDING) / (layerCount - 1),
          )
        : 0;

    layerIds.forEach((layerId, layerIndex) => {
      const layerNodes = layers[layerId];
      const x = LAYOUT_PADDING + layerIndex * xStep;

      let yStep = 0;
      if (layerNodes.length > 1) {
        yStep = Math.max(
          MIN_NODE_GAP_PX,
          (contentHeight - 2 * LAYOUT_PADDING) / (layerNodes.length - 1),
        );
      }

      const span = yStep * Math.max(layerNodes.length - 1, 0);
      const startY = (contentHeight - span) / 2;

      layerNodes.forEach((node, i) => {
        node.x = x;
        node.y = startY + i * yStep;
      });
    });

    // Calculate impacts
    const failureId = data.simulation.failureNode;
    const impacted = new Set<string>();
    const impactedLinks = new Set<string>();
    const queue = [failureId];

    while (queue.length > 0) {
      const currentId = queue.shift()!;
      data.roads.forEach((road) => {
        if (road.from === currentId) {
          if (!impacted.has(road.to)) {
            impacted.add(road.to);
            queue.push(road.to);
          }
          impactedLinks.add(`${road.from}-${road.to}`);
        }
      });
    }

    return {
      nodes,
      roads: data.roads.map((road) => ({
        ...road,
        source: nodes.find((n) => n.id === road.from)!,
        target: nodes.find((n) => n.id === road.to)!,
      })),
      impactedNodes: impacted,
      impactedRoads: impactedLinks,
      contentWidth,
      contentHeight,
    };
  }, [data]);

  useEffect(() => {
    setImpactedNodes(processedData.impactedNodes);
    setImpactedRoads(processedData.impactedRoads);
  }, [processedData]);

  const initialFitDone = useRef(false);

  // Zoom implementation
  useEffect(() => {
    if (!svgRef.current) return;

    const zoom = d3
      .zoom<SVGSVGElement, unknown>()
      .scaleExtent([0.05, 5])
      .on("zoom", (event) => {
        setTransform(event.transform);
      });

    zoomRef.current = zoom;
    const svg = d3.select(svgRef.current);
    // Attach zoom/pan to the SVG root.
    svg.call(zoom);

    const fitToScreen = (immediate = false) => {
      if (!svgRef.current) return;
      const container = svgRef.current.parentElement;
      if (!container) return;

      const { width: containerWidth, height: containerHeight } =
        container.getBoundingClientRect();
      if (containerWidth > 0 && containerHeight > 0) {
        const contentWidth = processedData.contentWidth;
        const contentHeight = processedData.contentHeight;
        const scale =
          Math.min(
            containerWidth / contentWidth,
            containerHeight / contentHeight,
          ) * 0.85;
        const x = (containerWidth - contentWidth * scale) / 2;
        const y = (containerHeight - contentHeight * scale) / 2;

        const t = d3.zoomIdentity.translate(x, y).scale(scale);
        if (immediate) {
          svg.call(zoom.transform, t);
        } else {
          svg.transition().duration(750).call(zoom.transform, t);
        }
      }
    };

    if (!initialFitDone.current) {
      setTimeout(() => fitToScreen(true), 100);
      initialFitDone.current = true;
    }
  }, [data, processedData.contentWidth, processedData.contentHeight]);

  const handleZoomIn = () => {
    if (!svgRef.current || !zoomRef.current) return;
    d3.select(svgRef.current)
      .transition()
      .duration(300)
      .call(zoomRef.current.scaleBy, 1.3);
  };

  const handleZoomOut = () => {
    if (!svgRef.current || !zoomRef.current) return;
    d3.select(svgRef.current)
      .transition()
      .duration(300)
      .call(zoomRef.current.scaleBy, 0.7);
  };

  const handleResetZoom = () => {
    if (!svgRef.current || !zoomRef.current) return;
    const container = svgRef.current.parentElement;
    if (!container) return;

    const { width: containerWidth, height: containerHeight } =
      container.getBoundingClientRect();
    const contentWidth = processedData.contentWidth;
    const contentHeight = processedData.contentHeight;
    const scale =
      Math.min(containerWidth / contentWidth, containerHeight / contentHeight) *
      0.85;
    const x = (containerWidth - contentWidth * scale) / 2;
    const y = (containerHeight - contentHeight * scale) / 2;

    d3.select(svgRef.current)
      .transition()
      .duration(750)
      .call(
        zoomRef.current.transform,
        d3.zoomIdentity.translate(x, y).scale(scale),
      );
  };

  if (!data) {
    return (
      <div className="min-h-screen flex items-center justify-center bg-white text-slate-700">
        Loading supply chain data...
      </div>
    );
  }

  const latestLossReport = outputData?.inventory_losses?.length
    ? outputData.inventory_losses[outputData.inventory_losses.length - 1]
    : null;

  const isSuggestionRoadLink = (from: string, to: string) => {
    if (!suggestionData?.implemented_links) return false;
    return (
      suggestionData.implemented_links.includes(`${from}-${to}`) ||
      suggestionData.implemented_links.includes(`${to}-${from}`)
    );
  };

  const getRoadMetrics = (road: Road) => {
    const time = road.distance + road.trafficDelay;
    const cost =
      (road.distance / data.config.truckMileage_kmpl) *
        data.config.petrolRate_INR +
      road.tollTax;
    const itemsSent = road.capacity;

    return { itemsSent, cost, time };
  };

  return (
    <div className="min-h-screen bg-white font-sans text-slate-900 overflow-hidden flex flex-col">
      {/* Header */}
      <header className="bg-white/80 backdrop-blur-md border-b border-slate-200 px-6 py-4 flex items-center justify-between z-10 shadow-sm">
        <div className="flex items-center gap-3">
          <div className="p-2 bg-indigo-600 rounded-lg shadow-lg">
            <Activity className="w-6 h-6 text-white" />
          </div>
          <div>
            <h1 className="text-xl font-black tracking-tighter text-slate-900">
              SUPPLYCHAIN <span className="text-indigo-600">CITY</span>
            </h1>
            <p className="text-[10px] text-slate-500 font-bold uppercase tracking-[0.2em]">
              Interactive Resilience Monitor
            </p>
          </div>
        </div>

        <div className="flex items-center gap-6">
          {/* Optimization Mode Dropdown */}
          {data.simulation.failureNode && (
            <div className="flex items-center gap-3 px-4 py-2 bg-slate-50 border border-slate-200 rounded-xl group hover:border-indigo-300 transition-all">
              <div className="p-1.5 bg-white rounded-lg shadow-sm border border-slate-100 group-hover:bg-indigo-50 transition-colors">
                <Zap className="w-4 h-4 text-indigo-600" />
              </div>
              <div className="flex flex-col">
                <span className="text-[9px] font-black text-slate-400 uppercase tracking-[0.2em]">
                  Optimization Mode
                </span>
                <div className="relative flex items-center">
                  <select
                    value={optimizationMode}
                    onChange={(e) => setOptimizationMode(e.target.value)}
                    className="appearance-none bg-transparent text-xs font-black text-slate-900 pr-6 focus:outline-none cursor-pointer"
                  >
                    {OPTIMIZATION_MODES.map((mode) => (
                      <option key={mode.id} value={mode.id}>
                        {mode.label}
                      </option>
                    ))}
                  </select>
                  <ChevronDown className="w-3 h-3 text-slate-400 absolute right-0 pointer-events-none" />
                </div>
              </div>
            </div>
          )}

          {/* Strategy Metrics in Header */}
          <AnimatePresence>
            {suggestionData && (
              <motion.div
                initial={{ opacity: 0, x: 20 }}
                animate={{ opacity: 1, x: 0 }}
                exit={{ opacity: 0, x: 20 }}
                className="flex items-center gap-4 border-l border-slate-200 pl-6"
              >
                <div className="flex flex-col">
                  <span className="text-[8px] font-black text-slate-400 uppercase tracking-widest">
                    Capacity
                  </span>
                  <span className="text-xs font-black text-emerald-600">
                    {suggestionData.capacity.toLocaleString()}
                  </span>
                </div>
                <div className="flex flex-col">
                  <span className="text-[8px] font-black text-slate-400 uppercase tracking-widest">
                    Cost
                  </span>
                  <span className="text-xs font-black text-emerald-600">
                    ₹{suggestionData.cost.toLocaleString()}
                  </span>
                </div>
                <div className="flex flex-col">
                  <span className="text-[8px] font-black text-slate-400 uppercase tracking-widest">
                    Time
                  </span>
                  <span className="text-xs font-black text-emerald-600">
                    {suggestionData.time}m
                  </span>
                </div>
              </motion.div>
            )}
          </AnimatePresence>

          <div className="flex items-center gap-4">
            <div className="flex items-center gap-2 px-4 py-2 bg-red-50 border border-red-100 rounded-xl">
              <AlertTriangle className="w-4 h-4 text-red-500 animate-pulse" />
              <span className="text-xs font-bold text-red-600 uppercase tracking-wider">
                Failure: {data.simulation.failureNode}
              </span>
            </div>
          </div>
        </div>
      </header>

      <main
        className="flex-1 relative bg-white cursor-grab active:cursor-grabbing overflow-hidden"
        onClick={() => {
          setHoveredNode(null);
          setHoveredRoad(null);
        }}
      >
        {/* Dotted Canvas Background */}
        <div
          className="absolute inset-0 pointer-events-none opacity-[0.4] z-0"
          style={{
            backgroundImage: "radial-gradient(#cbd5e1 1px, transparent 1px)",
            backgroundSize: "24px 24px",
            transform: `translate(${transform.x}px, ${transform.y}px) scale(${transform.k})`,
            transformOrigin: "0 0",
          }}
        />

        {/* Visualization Canvas */}
        <svg
          ref={svgRef}
          className="absolute inset-0 w-full h-full z-10"
          style={{ pointerEvents: "all", touchAction: "none" }}
        >
          <rect width="100%" height="100%" fill="transparent" />
          <defs>
            <filter id="glow" x="-50%" y="-50%" width="200%" height="200%">
              <feGaussianBlur stdDeviation="4" result="blur" />
              <feComposite in="SourceGraphic" in2="blur" operator="over" />
            </filter>
          </defs>
          <g
            transform={`translate(${transform.x},${transform.y}) scale(${transform.k})`}
          >
            {/* Roads */}
            <g>
              {processedData.roads.map((road) => {
                const isImpacted = impactedRoads.has(`${road.from}-${road.to}`);
                const source = road.source;
                const target = road.target;

                // Check if this road is part of the suggested path
                const isSuggestionRoad = isSuggestionRoadLink(
                  road.from,
                  road.to,
                );

                const roadMetrics = getRoadMetrics(road);

                const isDimmed = suggestionData && !isSuggestionRoad;

                const midX = (source.x + target.x) / 2;
                const midY = (source.y + target.y) / 2;

                return (
                  <g
                    key={`road-${road.from}-${road.to}`}
                    className="group"
                    onClick={(e) => {
                      e.stopPropagation();
                      setHoveredRoad(
                        hoveredRoad?.from === road.from &&
                          hoveredRoad?.to === road.to
                          ? null
                          : road,
                      );
                      setHoveredNode(null);
                    }}
                    onMouseEnter={() =>
                      !hoveredRoad && !hoveredNode && setHoveredRoad(road)
                    }
                    onMouseLeave={() =>
                      !hoveredRoad && !hoveredNode && setHoveredRoad(null)
                    }
                  >
                    <line
                      x1={source.x}
                      y1={source.y}
                      x2={target.x}
                      y2={target.y}
                      stroke="transparent"
                      strokeWidth={20}
                      className="cursor-help"
                    />
                    <line
                      x1={source.x}
                      y1={source.y}
                      x2={target.x}
                      y2={target.y}
                      stroke={
                        isSuggestionRoad
                          ? "#bbf7d0"
                          : isImpacted
                            ? "#fee2e2"
                            : "#f1f5f9"
                      }
                      strokeWidth={isSuggestionRoad ? 20 : 8}
                      strokeLinecap="round"
                    />
                    <line
                      x1={source.x}
                      y1={source.y}
                      x2={target.x}
                      y2={target.y}
                      stroke={
                        isSuggestionRoad
                          ? "#22c55e"
                          : isImpacted
                            ? "#fca5a5"
                            : "#cbd5e1"
                      }
                      strokeWidth={isSuggestionRoad ? 8 : 2}
                      strokeDasharray={isImpacted ? "4,4" : "none"}
                      style={
                        isSuggestionRoad
                          ? { filter: "drop-shadow(0 0 8px #22c55e)" }
                          : {}
                      }
                      className="transition-all duration-300"
                    />
                    {isSuggestionRoad && (
                      <>
                        <circle r="5" fill="#16a34a">
                          <animateMotion
                            dur="1.2s"
                            repeatCount="indefinite"
                            path={`M ${source.x} ${source.y} L ${target.x} ${target.y}`}
                          />
                        </circle>

                        {/* Green route metrics label */}
                        <g transform={`translate(${midX}, ${midY})`}>
                          <rect
                            x={-80}
                            y={-24}
                            width={160}
                            height={32}
                            rx={8}
                            fill="rgba(240,253,244,0.95)"
                            stroke="#86efac"
                            strokeWidth={1}
                          />
                          <text
                            x={0}
                            y={-11}
                            textAnchor="middle"
                            className="fill-emerald-700 text-[9px] font-black uppercase tracking-wide"
                          >
                            Items: {roadMetrics.itemsSent}
                          </text>
                          <text
                            x={0}
                            y={2}
                            textAnchor="middle"
                            className="fill-emerald-700 text-[9px] font-black uppercase tracking-wide"
                          >
                            Cost: ₹
                            {Math.round(roadMetrics.cost).toLocaleString()}
                          </text>
                          <text
                            x={0}
                            y={15}
                            textAnchor="middle"
                            className="fill-emerald-700 text-[9px] font-black uppercase tracking-wide"
                          >
                            Time: {roadMetrics.time}m
                          </text>
                        </g>
                      </>
                    )}
                    {!isImpacted && !isSuggestionRoad && (
                      <circle r="3" fill="#6366f1" opacity={0.6}>
                        <animateMotion
                          dur={`${Math.max(2, road.distance / 40)}s`}
                          repeatCount="indefinite"
                          path={`M ${source.x} ${source.y} L ${target.x} ${target.y}`}
                        />
                      </circle>
                    )}
                  </g>
                );
              })}
            </g>

            {/* Nodes */}
            <g>
              {processedData.nodes.map((node) => {
                const isFailed =
                  node.id === data.simulation.failureNode ||
                  suggestionData?.unrecoverable_nodes?.includes(node.id);
                const isImpacted = impactedNodes.has(node.id) && !isFailed;

                // Check if node is part of the suggested optimization
                const isSuggested =
                  suggestionData?.path?.includes(node.id) ||
                  suggestionData?.implemented_links?.some((link: string) =>
                    link.includes(node.id),
                  );

                const isDimmed = suggestionData && !isSuggested;
                const Icon = NODE_ICONS[node.type];

                return (
                  <g
                    key={node.id}
                    transform={`translate(${node.x},${node.y})`}
                    className="cursor-pointer group"
                    onClick={(e) => {
                      e.stopPropagation();
                      setHoveredNode(hoveredNode?.id === node.id ? null : node);
                      setHoveredRoad(null);
                    }}
                    onMouseEnter={() =>
                      !hoveredNode && !hoveredRoad && setHoveredNode(node)
                    }
                    onMouseLeave={() =>
                      !hoveredNode && !hoveredRoad && setHoveredNode(null)
                    }
                  >
                    {isFailed || isImpacted || isSuggested ? (
                      <circle
                        r={70}
                        fill="transparent"
                        stroke={
                          isFailed
                            ? "#ef4444"
                            : isImpacted
                              ? "#f97316"
                              : "#22c55e"
                        }
                        strokeWidth={isSuggested ? 8 : 3}
                        className="animate-pulse"
                        style={{
                          animationDuration: "1s",
                          filter: isSuggested
                            ? "drop-shadow(0 0 12px #22c55e)"
                            : "none",
                        }}
                      />
                    ) : null}
                    <circle
                      r={45}
                      fill="white"
                      stroke={
                        isFailed
                          ? "#ef4444"
                          : isImpacted
                            ? "#f97316"
                            : isSuggested
                              ? "#22c55e"
                              : "#e2e8f0"
                      }
                      strokeWidth={isSuggested ? 8 : 2}
                      className="transition-all duration-300 shadow-sm group-hover:shadow-md group-hover:scale-110"
                    />
                    <g transform="translate(-16,-16)">
                      <Icon
                        className={cn(
                          "w-8 h-8 transition-colors duration-300",
                          isFailed
                            ? "text-red-500"
                            : isImpacted
                              ? "text-orange-500"
                              : isSuggested
                                ? "text-emerald-600"
                                : "text-slate-600",
                        )}
                      />
                    </g>
                    <text
                      dy={75}
                      textAnchor="middle"
                      className={cn(
                        "text-[12px] font-black uppercase tracking-wider pointer-events-none transition-colors",
                        isSuggested ? "fill-emerald-700" : "fill-slate-500",
                      )}
                    >
                      {node.id.replace(/_/g, " ")}
                    </text>
                  </g>
                );
              })}
            </g>
          </g>
        </svg>

        {/* Zoom Controls */}
        <div className="absolute top-8 left-8 flex flex-col gap-2 pointer-events-auto">
          <ZoomButton
            onClick={handleZoomIn}
            icon={<Plus className="w-4 h-4" />}
          />
          <ZoomButton
            onClick={handleZoomOut}
            icon={<Minus className="w-4 h-4" />}
          />
          <ZoomButton
            onClick={handleResetZoom}
            icon={<Maximize className="w-4 h-4" />}
          />
        </div>

        {/* Floating UI Overlay */}
        <div className="absolute inset-0 pointer-events-none z-20">
          <AnimatePresence>
            {hoveredNode && (
              <motion.div
                initial={{ opacity: 0, scale: 0.95, y: 20 }}
                animate={{ opacity: 1, scale: 1, y: 0 }}
                exit={{ opacity: 0, scale: 0.95, y: 20 }}
                className="absolute top-8 right-8 w-80 pointer-events-auto bg-white/95 backdrop-blur-2xl border border-slate-200 rounded-3xl shadow-2xl overflow-hidden"
              >
                <div className="p-6 border-b border-slate-100 bg-slate-50/50">
                  <div className="flex items-center justify-between mb-2">
                    <span className="text-[10px] font-bold text-indigo-600 uppercase tracking-widest">
                      {hoveredNode.type}
                    </span>
                    <div
                      className={cn(
                        "px-2 py-0.5 rounded-full text-[9px] font-black uppercase tracking-tighter",
                        hoveredNode.id === data.simulation.failureNode
                          ? "bg-red-100 text-red-600"
                          : impactedNodes.has(hoveredNode.id)
                            ? "bg-orange-100 text-orange-600"
                            : "bg-emerald-100 text-emerald-600",
                      )}
                    >
                      {hoveredNode.id === data.simulation.failureNode
                        ? "Offline"
                        : impactedNodes.has(hoveredNode.id)
                          ? "Impacted"
                          : "Active"}
                    </div>
                  </div>
                  <h3 className="text-lg font-black text-slate-900 tracking-tight">
                    {hoveredNode.id.replace(/_/g, " ")}
                  </h3>
                </div>
                <div className="p-6 space-y-4">
                  <p className="text-xs font-medium text-slate-600 leading-relaxed">
                    {hoveredNode.id === data.simulation.failureNode
                      ? "This facility has experienced a complete shutdown. All outbound logistics are halted."
                      : impactedNodes.has(hoveredNode.id)
                        ? "Supply chain disruption detected. This node is receiving limited or no input from primary sources."
                        : "Node is operating at full capacity. Real-time monitoring shows no anomalies."}
                  </p>

                  {hoveredNode.inventory &&
                    hoveredNode.inventory.length > 0 && (
                      <div className="pt-4 border-t border-slate-100">
                        <h4 className="text-[9px] font-black text-slate-400 uppercase tracking-[0.2em] mb-3">
                          Inventory Status
                        </h4>
                        <div className="space-y-2">
                          {hoveredNode.inventory.map((item, idx) => (
                            <div
                              key={`${item.item}-${idx}`}
                              className="flex items-center justify-between bg-slate-50 p-2 rounded-lg border border-slate-100"
                            >
                              <span className="text-[10px] font-bold text-slate-700">
                                {item.item.replace(/_/g, " ")}
                              </span>
                              <span className="text-[10px] font-black text-indigo-600">
                                {item.quantity} {item.unit}
                              </span>
                            </div>
                          ))}
                        </div>
                      </div>
                    )}
                </div>
              </motion.div>
            )}

            {hoveredRoad && (
              <motion.div
                initial={{ opacity: 0, x: 20 }}
                animate={{ opacity: 1, x: 0 }}
                exit={{ opacity: 0, x: 20 }}
                className="absolute bottom-8 right-8 w-80 pointer-events-auto bg-white/95 backdrop-blur-2xl border border-slate-200 rounded-3xl shadow-2xl p-6"
              >
                {(() => {
                  const suggestionRoad = isSuggestionRoadLink(
                    hoveredRoad.from,
                    hoveredRoad.to,
                  );
                  const metrics = getRoadMetrics(hoveredRoad);

                  return (
                    <>
                      <div className="flex items-center gap-2 mb-6">
                        <ArrowRight className="w-4 h-4 text-indigo-500" />
                        <h3 className="text-[10px] font-black text-slate-500 uppercase tracking-[0.2em]">
                          Logistics Data
                        </h3>
                      </div>
                      {suggestionRoad && (
                        <div className="mb-4 grid grid-cols-3 gap-3 bg-emerald-50 border border-emerald-100 rounded-xl p-3">
                          <LogItem
                            label="Items Sent"
                            value={`${metrics.itemsSent} units`}
                          />
                          <LogItem
                            label="Cost"
                            value={`₹${Math.round(metrics.cost).toLocaleString()}`}
                          />
                          <LogItem label="Time" value={`${metrics.time} min`} />
                        </div>
                      )}
                      <div className="mb-6 space-y-2">
                        <div className="flex items-center justify-between text-[10px] font-bold">
                          <span className="text-slate-400 uppercase tracking-wider">
                            From
                          </span>
                          <span className="text-slate-900">
                            {hoveredRoad.from.replace(/_/g, " ")}
                          </span>
                        </div>
                        <div className="flex items-center justify-between text-[10px] font-bold">
                          <span className="text-slate-400 uppercase tracking-wider">
                            To
                          </span>
                          <span className="text-slate-900">
                            {hoveredRoad.to.replace(/_/g, " ")}
                          </span>
                        </div>
                      </div>
                      <div className="grid grid-cols-2 gap-6">
                        <LogItem
                          label="Distance"
                          value={`${hoveredRoad.distance} km`}
                        />
                        <LogItem
                          label="Capacity"
                          value={`${hoveredRoad.capacity} units`}
                        />
                        <LogItem
                          label="Toll"
                          value={`₹${hoveredRoad.tollTax}`}
                        />
                        <LogItem
                          label="Delay"
                          value={`${hoveredRoad.trafficDelay} min`}
                        />
                      </div>
                    </>
                  );
                })()}
              </motion.div>
            )}
          </AnimatePresence>
        </div>
      </main>

      <footer className="bg-white border-t border-slate-200 px-8 py-4 flex items-center justify-between z-10 overflow-hidden">
        <div className="flex items-center gap-12 shrink-0">
          <FooterStat label="Total Nodes" value={data.nodes.length} />
          <FooterStat
            label="Impacted"
            value={impactedNodes.size}
            color="text-orange-600"
          />
          <FooterStat
            label="Fuel Rate"
            value={`₹${data.config.petrolRate_INR}`}
          />
          <FooterStat
            label="Efficiency"
            value={`${data.config.truckMileage_kmpl} km/l`}
          />
        </div>

        <div className="flex items-center gap-4 border-l border-slate-100 pl-8 overflow-hidden">
          <div className="flex flex-col shrink-0">
            <span className="text-[9px] font-black text-red-400 uppercase tracking-[0.2em]">
              Inventory Loss
            </span>
            <span className="text-xs font-black text-red-600">
              {latestLossReport?.entries?.length ?? 0} Items Lost
            </span>
          </div>

          <div className="flex gap-3 overflow-x-auto no-scrollbar py-1">
            {(latestLossReport?.entries ?? []).map((loss, idx) => (
              <div
                key={`${loss.node}-${loss.item}-${idx}`}
                className="flex items-center gap-2 bg-red-50 border border-red-100 px-3 py-1.5 rounded-lg shrink-0"
              >
                <div className="w-1.5 h-1.5 rounded-full bg-red-500" />
                <span className="text-[10px] font-bold text-red-900 whitespace-nowrap">
                  {loss.item.replace(/_/g, " ")}:{" "}
                  <span className="font-black">
                    {loss.quantity_lost} {loss.unit}
                  </span>
                </span>
                <span className="text-[9px] font-medium text-red-400 italic">
                  @ {loss.node.replace(/_/g, " ")}
                </span>
              </div>
            ))}
          </div>
        </div>
      </footer>
    </div>
  );
}

const LogItem = ({
  label,
  value,
}: {
  label: string;
  value: string | number;
}) => (
  <div>
    <p className="text-[10px] font-bold text-slate-400 uppercase tracking-wider mb-1">
      {label}
    </p>
    <p className="text-sm font-black text-slate-900">{value}</p>
  </div>
);

const FooterStat = ({
  label,
  value,
  color = "text-slate-900",
}: {
  label: string;
  value: string | number;
  color?: string;
}) => (
  <div className="flex flex-col">
    <span className="text-[9px] font-black text-slate-400 uppercase tracking-[0.2em]">
      {label}
    </span>
    <span className={cn("text-xs font-black", color)}>{value}</span>
  </div>
);

const ZoomButton = ({
  onClick,
  icon,
}: {
  onClick: () => void;
  icon: React.ReactNode;
}) => (
  <button
    onClick={onClick}
    className="w-10 h-10 bg-white border border-slate-200 rounded-xl shadow-sm flex items-center justify-center text-slate-600 hover:bg-slate-50 hover:text-indigo-600 transition-all active:scale-95"
  >
    {icon}
  </button>
);
